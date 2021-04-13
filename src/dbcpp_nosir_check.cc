#define NICKNAME "MAXIM"
#include <serverlib/slogger.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/dbcpp_cursctl.h>
#include "dbcpp_nosir_check.h"
#include <serverlib/pg_cursctl.h>
#include <serverlib/testmode.h>

namespace DbCpp
{

static bool checkTestRowCount(const std::string& nick, const char* file, size_t line,
                              std::vector<TCheckSessionsLoadSaveConsistency>& result,
                              DbCpp::Session& session, int value, const std::string& error)
{
  int count = 0;
  DbCpp::CursCtl(session, STDLOG, "select count(1) as count from TEST_AUTONOMOUS_SESSION", false)
    .def(count).EXfet();

  if (count != value) {
    result.push_back({STDLOG_VARIABLE, "Invalid loaded data, db row count("
                      + std::to_string(count) + ") != " + std::to_string(value)
                      + ": " + error});
    return false;
  }
  return true;
}

std::vector<TCheckSessionsLoadSaveConsistency> check_autonomous_sessions_load_save_consistency()
{
  // inTestMode: Session AUTO == MAIN!

  LogTrace(TRACE1) << __func__;
  std::vector<TCheckSessionsLoadSaveConsistency> result;

  // ������ ���㦥��� � �⤥�쭮� ��ᨨ
  PgCpp::SessionDescriptor acs = PgCpp::getAutoCommitSession(readStringFromTcl("PG_CONNECT_STRING"));
  make_pg_curs_autonomous(acs, "drop table if exists TEST_AUTONOMOUS_SESSION").exec();
  make_pg_curs_autonomous(acs, "create table TEST_AUTONOMOUS_SESSION ( FLD1 varchar(1) not null );").exec();
  make_pg_curs_autonomous(acs, "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('1')").exec();

  // �஢��塞 ࠡ��� RO-��ᨨ, RW-��ᨨ
  // 1
  LogTrace(TRACE1) << "ro=" << &DbCpp::mainPgReadOnlySession(STDLOG);
  if (!checkTestRowCount(STDLOG, result,
                         DbCpp::mainPgReadOnlySession(STDLOG), 1,
                         "Main Session: First record"))
  {
    return result;
  }

  // ������塞 ������ � �᭮���� ��ᨨ (������ �ந�室�� � ���� ���)
  LogTrace(TRACE1) << "rw=" << &DbCpp::mainPgManagedSession(STDLOG);
  DbCpp::CursCtl(DbCpp::mainPgManagedSession(STDLOG), STDLOG,
                 "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('2')", false).exec();


  // 1, Main: 2
  LogTrace(TRACE1) << "ro=" << &DbCpp::mainPgReadOnlySession(STDLOG);
  if (!checkTestRowCount(STDLOG, result,
                         DbCpp::mainPgReadOnlySession(STDLOG), 2,
                         "Main Session: After insert record"))
  {
    return result;
  }

  // �஢��塞 ࠡ��� ��⮭����� (��������� ������ ����������� � �᭮���� ��ᨨ)
  // 1
  // inTestMode: 2
  LogTrace(TRACE1) << "au=" << &DbCpp::mainPgAutonomousSession(STDLOG);
  if (!checkTestRowCount(STDLOG, result,
                         DbCpp::mainPgAutonomousSession(STDLOG), inTestMode() ? 2 : 1,
                         "Auto Session: After insert record"))
  {
    return result;
  }

  // �஢��塞 ࠡ��� �������� ��⮭����� ��ᨨ
  // ������塞 ���� �����, �஢��塞, �� ����� ����� ⮫쪮 � ⮩ ��ᨨ, ��� ��� �뫨 ᮧ����
  // �஢��塞, �� ��������� �訡�� �� ���饭�� � ��⮭����� ��ᨨ �� �१ ��������
  {
    DbCpp::PgAutonomousSessionManager mngr1 = DbCpp::mainPgAutonomousSessionManager(STDLOG);
    LogTrace(TRACE1) << "mngr au=" << &mngr1.session();
    DbCpp::CursCtl(mngr1.session(), STDLOG,
                   "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('A')", false).exec();
    DbCpp::CursCtl(mngr1.session(), STDLOG,
                   "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('B')", false).exec();

    // 1, Auto: A, B
    // inTestMode: 1, 2, A, B
    LogTrace(TRACE1) << "mngr au=" << &mngr1.session();
    if (!checkTestRowCount(STDLOG, result,
                           mngr1.session(), inTestMode() ? 4 : 3,
                           "Auto Session: Before commit"))
    {
      return result;
    }
    // 1, Main: 2
    // inTestMode: 1, 2, A, B
    LogTrace(TRACE1) << "ro=" << &DbCpp::mainPgReadOnlySession(STDLOG);
    if (!checkTestRowCount(STDLOG, result,
                           DbCpp::mainPgReadOnlySession(STDLOG), inTestMode() ? 4 : 2,
                           "Main Session: Before commit"))
    {
      return result;
    }

    try {
        DbCpp::CursCtl(DbCpp::mainPgAutonomousSession(STDLOG), STDLOG,
                       "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('E')", false).exec();
        return {{STDLOG, "Not finished managed session changed to autonomous"}};
    } catch (const comtech::Exception& e) {
        const std::string check = "Autonomous session in managed mode, use autonomous session manager";
        const std::string error = e.what();
        if (error.find(check) == error.npos) {
            throw e;
        }
    }

    // �஢��塞 �� ���� ��᫥ ������, ���� ���� ��������,
    // ���饭�� ������� � ��⮭����� ��ᨨ ����饭�.

    mngr1.commit();

    try {
        DbCpp::CursCtl(DbCpp::mainPgAutonomousSession(STDLOG), STDLOG,
                       "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('E')", false).exec();
        return {{STDLOG, "Not finished managed session changed to autonomous"}};
    } catch (const comtech::Exception& e) {
        const std::string check = "Autonomous session in managed mode, use autonomous session manager";
        const std::string error = e.what();
        if (error.find(check) == error.npos) {
            throw e;
        }
    }

    // �஢��塞 �� ��᫥ ������ ��࠭�� ����� ����� �� ��� �����
    // � �� �᭮���� ���� �� �������祭�.

    // 1, A, B
    // inTestMode: 1, 2, A, B
    LogTrace(TRACE1) << "mngr au=" << &mngr1.session();
    if (!checkTestRowCount(STDLOG, result,
                           mngr1.session(), inTestMode() ? 4 : 3,
                           "Auto Session: After commit"))
    {
      return result;
    }

    // 1, A, B, Main: 2
    LogTrace(TRACE1) << "ro=" << &DbCpp::mainPgReadOnlySession(STDLOG);
    if (!checkTestRowCount(STDLOG, result,
                           DbCpp::mainPgReadOnlySession(STDLOG), 4,
                           "Main Session: After commit"))
    {
      return result;
    }

    // �஢��塞 ����� �� ᮧ����� ��ண� ��������, ���� ���� ����

    try {
        DbCpp::PgAutonomousSessionManager mngr2 = DbCpp::mainPgAutonomousSessionManager(STDLOG);
        DbCpp::CursCtl(mngr2.session(), STDLOG,
                       "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('C')", false).exec();
        return {{STDLOG, "Not finished managed session changed to autonomous"}};
    } catch (const comtech::Exception& e) {
        const std::string check = "Autonomous session in managed mode, use autonomous session manager";
        const std::string error = e.what();
        if (error.find(check) == error.npos) {
            throw e;
        }
    }

    // ������塞 ���� ����� � ��⮭����� ��ᨨ ��� �஢�ન �⪠�

    DbCpp::CursCtl(mngr1.session(), STDLOG,
                   "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('C')", false).exec();
    DbCpp::CursCtl(mngr1.session(), STDLOG,
                   "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('D')", false).exec();

    // 1, A, B, Auto: C, D
    // inTestMode: 1, 2, A, B, C, D
    if (!checkTestRowCount(STDLOG, result,
                           mngr1.session(), inTestMode() ? 6 : 5,
                           "Auto Session: Before auto rollback"))
    {
      return result;
    }
    // 1, A, B, Main: 2
    // inTestMode: 1, 2, A, B, C, D
    if (!checkTestRowCount(STDLOG, result,
                           DbCpp::mainPgReadOnlySession(STDLOG), inTestMode() ? 6 : 4,
                           "Main Session: Before auto rollback"))
    {
      return result;
    }
  } // �⪠� �� ࠧ��襭�� ��������

  // �஢��塞, �� �᭮���� ���� �� �⪠⨫���

  // 1, A, B, Main: 2
  if (!checkTestRowCount(STDLOG, result,
                         DbCpp::mainPgReadOnlySession(STDLOG), 4,
                         "Main Session: After auto rollback"))
  {
    return result;
  }

  // �஢��塞, �� ��� �������� ����㯭� ��⮭���� �࠭���樨

  // 1, A, B
  // inTestMode: 1, 2, A, B
  if (!checkTestRowCount(STDLOG, result,
                         DbCpp::mainPgAutonomousSession(STDLOG), inTestMode() ? 4 : 3,
                         "Auto Session: After main rollback"))
  {
    return result;
  }

  // �஢��塞, �� ����� ᮧ���� ���� ��������

  {
    DbCpp::PgAutonomousSessionManager mngr2 = DbCpp::mainPgAutonomousSessionManager(STDLOG);
    DbCpp::CursCtl(mngr2.session(), STDLOG,
                   "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('F')", false).exec();
    DbCpp::CursCtl(mngr2.session(), STDLOG,
                   "insert into TEST_AUTONOMOUS_SESSION(FLD1) VALUES ('J')", false).exec();

    // �⪠�뢠�� �᭮���� ���� � �஢��塞 �� ����� � �᭮���� ��ᨨ �⬥����

    DbCpp::mainPgManagedSession(STDLOG).rollback();

    // 1, A, B
    // inTestMode: 1, 2, A, B
    if (!checkTestRowCount(STDLOG, result,
                           DbCpp::mainPgReadOnlySession(STDLOG), inTestMode() ? 4 : 3,
                           "Main Session: After main rollback"))
    {
      return result;
    }

    // �஢��塞 �� ����� � ��⮭����� ��ᨨ �� �⪠⨫���

    // 1, A, B, Auto: F, J
    // inTestMode: 1, 2, A, B
    if (!checkTestRowCount(STDLOG, result,
                           mngr2.session(), inTestMode() ? 4 : 5,
                           "Auto Session: After main rollback"))
    {
      return result;
    }
  } // �⪠� �� ࠧ��襭�� ��������

  // �஢��塞 �� �� ����� �⪠⨫���

  // 1, A, B
  // inTestMode: 1, 2, A, B
  if (!checkTestRowCount(STDLOG, result,
                         DbCpp::mainPgReadOnlySession(STDLOG), inTestMode() ? 4 : 3,
                         "Main Session: After auto rollback"))
  {
    return result;
  }

  // 1, A, B
  // inTestMode: 1, 2, A, B
  if (!checkTestRowCount(STDLOG, result,
                         DbCpp::mainPgAutonomousSession(STDLOG), inTestMode() ? 4 : 3,
                         "Auto Session: After auto rollback"))
  {
    return result;
  }

  return result;
}

} // DbCpp

