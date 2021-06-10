#pragma once
#ifdef ENABLE_PG

#include <memory>
#include <string>
#include <vector>

#include "dbcpp_db_type.h"

#ifdef ENABLE_ORACLE
namespace OciCpp
{
    class OciSession;
    class CursCtl;
}
#endif

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
#ifdef XP_TESTING
        virtual void commitInTestMode() = 0;
#endif // XP_TESTING

        virtual std::string getConnectString() const = 0;

        virtual CopyResult copyDataFrom(const std::string& sql, const char* data, size_t size) = 0;

        bool isOracle() const { return getType() == DbType::Oracle; }
    };
#ifdef ENABLE_ORACLE
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
#ifdef XP_TESTING
        virtual void commitInTestMode() override;
#endif // XP_TESTING
        virtual std::string getConnectString() const override;
        virtual CopyResult copyDataFrom(const std::string& sql, const char* data,
                                        size_t size) override;

    private:
        OciCpp::CursCtl createOraCursor(const char* n, const char* f, int l, const char* sql,
                                        bool cacheit);

        std::unique_ptr<OciCpp::OciSession> mSession;
        bool mReleaseOnDestruction;
    };
#endif

    class PgSession_wo_CheckSQL final : public Session
    {
        friend class PostgresImpl;

    public:
        PgSession_wo_CheckSQL(const char* nick, const char* file, int line, const std::string& connStr,
                  DbSessionType type);
        PgSession_wo_CheckSQL(const std::string& connStr, DbSessionType type);
        PgSession_wo_CheckSQL(const char* connStr, DbSessionType type);

        ~PgSession_wo_CheckSQL();
        PgSession_wo_CheckSQL(const PgSession_wo_CheckSQL&) = delete;
        PgSession_wo_CheckSQL(PgSession_wo_CheckSQL&&)      = delete;
        PgSession_wo_CheckSQL& operator=(const PgSession_wo_CheckSQL&) = delete;
        PgSession_wo_CheckSQL& operator=(PgSession_wo_CheckSQL&&) = delete;

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
        virtual void commitInTestMode() override;
        void rollbackInTestMode();
#endif // XP_TESTING
        virtual std::string getConnectString() const override { return mConnectString; }
        void setClientInfo(const std::string& clientInfo);

        virtual CopyResult copyDataFrom(const std::string& sql, const char* data,
                                        size_t size) override;

        void activateSession();

        void setForReading();
        void setForManagedReadWrite();
        void setForAutonomousReadWrite();

        PgCpp::CursCtl createPgCursor(const char* n, const char* f, int l, const std::string& sql,
                                      bool cacheit);
    private:
        DbSessionType mType;
        bool mIsActive;
#ifdef XP_TESTING
        DbSessionType mType_test;
        bool mIsActive_test;
#endif // XP_TESTING
        DbSessionForType mForType;
        std::string mConnectString;
        std::shared_ptr<PgCpp::Session> mSession;

        bool setSessionType(DbSessionType type, bool no_throw);
    };

    class PgSession final : public Session
    {
    public:
        PgSession(PgSession_wo_CheckSQL* s,
                  DbSessionForType sess_for_type);
        PgSession(const std::string& connStr, DbSessionType type);

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

        virtual DbType getType() const override { return mPgSession->getType(); }
        virtual void commit() override { return mPgSession->commit(); }
        virtual void rollback() override { return mPgSession->rollback(); }
#ifdef XP_TESTING
        virtual void commitInTestMode() override { return mPgSession->commitInTestMode(); }
        void rollbackInTestMode() { return mPgSession->rollbackInTestMode(); }
#endif // XP_TESTING

        virtual CopyResult copyDataFrom(const std::string& sql, const char* data,
                                        size_t size) override {
                return mPgSession->copyDataFrom(sql,data,size); }

        void activateSession() { mPgSession->activateSession(); }

        virtual std::string getConnectString() const override { return mPgSession->getConnectString(); }

        PgCpp::CursCtl createPgCursor(const char* n, const char* f, int l, const std::string& sql,
                                      bool cacheit);
    private:
        DbSessionForType mForType;
        PgSession_wo_CheckSQL* mPgSession;
        void prepareSession(const char* nick, const char* file, int line);
        void prepareCursor(const char* nick, const char* file, int line,const std::string& sql);
        bool mDelete;

        friend class PgAutonomousSessionManager;
    };

#ifdef ENABLE_ORACLE
    OraSession& mainOraSession(const char* nick, const char* file, int line);
    OraSession* mainOraSessionPtr(const char* nick, const char* file, int line);
#endif

    PgSession& mainPgManagedSession(const char* nick, const char* file, int line);
    PgSession* mainPgManagedSessionPtr(const char* nick, const char* file, int line,
                                bool createIfNotExist = true);
    // ����� ⮫쪮 ��� �⥭��
    // �ᯮ���� �� �� pg backend (����), �� � mainPgManagedSession
    // �᫨ �࠭����� 㦥 ���� (� mainPgManagedSession), ࠡ�⠥� � ⮩ �� �࠭���樨
    // �᫨ �ࠧ����� �� ����, ࠡ�⠥� � ��⮭����� ०���
    // �ணࠬ��� ����஫������, �� sql-����� ⮫쪮 �� �⥭��
    PgSession& mainPgReadOnlySession(const char* nick, const char* file, int line);
    PgSession* mainPgReadOnlySessionPtr(const char* nick, const char* file, int line,
                                bool createIfNotExist = true);

    // ��⮭����� ����
    // � ०��� ��⮢ (inTestMode()) �ᯮ������  mainPgManagedSession
    // � ��⨢��� ��砥 ᮧ������ �⤥��� pg backend (����)
    PgSession& mainPgAutonomousSession(const char* nick, const char* file, int line);
    PgSession* mainPgAutonomousSessionPtr(const char* nick, const char* file, int line,
                                bool createIfNotExist = true);

    class PgAutonomousSessionManager
    {
    public:
        static PgAutonomousSessionManager start(const char* nick, const char* file, int line);
        ~PgAutonomousSessionManager();

        PgAutonomousSessionManager(const PgAutonomousSessionManager&) = delete;
        PgAutonomousSessionManager(PgAutonomousSessionManager&&) = delete;
        PgAutonomousSessionManager& operator=(const PgAutonomousSessionManager&) = delete;
        PgAutonomousSessionManager& operator=(Session&&) = delete;

        PgSession& session();

        void commit();
        void rollback();

    private:
        PgAutonomousSessionManager(PgSession& session);
        PgSession& mSession;
    };

    PgAutonomousSessionManager mainPgAutonomousSessionManager(const char* nick, const char* file, int line);


} // namespace DbCpp

#endif // ENABLE_PG
