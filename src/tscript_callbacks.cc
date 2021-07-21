#include "config.h"
#ifdef XP_TESTING

#include <serverlib/tscript.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/str_utils.h>
#ifdef ENABLE_ORACLE
#include <serverlib/cursctl.h>
#endif
#include <serverlib/testmode.h>
#include <serverlib/dbcpp_session.h>
#include <serverlib/commit_rollback.h>
#include <libtlg/tlg_outbox.h>
#include <jxtlib/xml_tools.h>
#include <jxtlib/utf2cp866.h>
#include <edilib/edi_func_cpp.h>

#include "oralib.h"
#include "hooked_session.h"
#include "pg_session.h"
#include "xp_testing.h"

#define NICKNAME "DMITRYVM"
#define NICKTRACE DMITRYVM_TRACE
#include <serverlib/slogger.h>

void tests_init();
void setLastRedisplay(const std::string &redisplay);

namespace xp_testing { namespace tscript {

    void CollectTlgs(const xp_testing::TlgAccumulator& tlgAccumulator, std::queue<std::string>& outq)
    {
        const std::vector<xp_testing::OutgoingTlg> outList = tlgAccumulator.list();
        /* ��ᯥ��뢠�� � ���������� ��室�騥 ⥣ࠬ�� */
        LogTrace(TRACE5) << __FUNCTION__ << ": " << outList.size() << " out tlgs";
        for (size_t i = 0; i < outList.size(); ++i) {
            LogTrace(TRACE5) << __FUNCTION__ << ": out[" << i << "] = \n" << outList[i].text();

            char *buff=0;
            size_t len;
            std::string edifact_sire = outList[i].text();
            if(!outList[i].text().empty()) {
                if(!ChangeEdiCharSet(outList[i].text().c_str(), outList[i].text().length(), "SIRE", &buff, &len) && buff) {
                    edifact_sire = std::string(buff,len);
                    free(buff);
                }
            }

            outq.push(edifact_sire);
        }
    }

    struct TsCallbacksImpl: public TsCallbacks {
        TsCallbacksImpl()
        {
            tests_init();
            SetTsCallbacks(this);
        }

        virtual void beforeFunctionCall()
        {
#ifdef ENABLE_ORACLE
            OciCpp::mainSession().set7();
            OraSession.Initialize(OciCpp::mainSession().getLd());
#endif //ENABLE_ORACLE
            tlgAccumulator_.clear();
            ServerFramework::listenRedisplay();
        }

        virtual void afterFunctionCall()
        {
            std::queue<std::string>& outq = GetTestContext()->outq;
            CollectTlgs(tlgAccumulator_, outq);
            if(!ServerFramework::getRedisplay().empty()) {
                LogTrace(TRACE5) << __FUNCTION__ << ": redisplay";
                std::string redisplay = ServerFramework::getRedisplay();
                if(redisplay.find("<?xml") == 0) {
                    redisplay = formatXmlString(redisplay);
                    redisplay = StrUtils::replaceSubstrCopy(redisplay, "\"", "'");
                    redisplay = UTF8toCP866(redisplay);
                    redisplay = StrUtils::replaceSubstrCopy(redisplay, "encoding='UTF-8'", "encoding='CP866'");
                }
                setLastRedisplay(redisplay);
                outq.push(redisplay);
            }
        }

        virtual void beforeTest()
        {
#ifdef ENABLE_ORACLE
            LogTrace(TRACE3) << __func__ << " tscript ************* savepoint tscript";
            make_curs("savepoint tscript").exec();
#endif //ENABLE_ORACLE
            TsCallbacks::beforeTest();
        }

        virtual void afterTest()
        {
            if(not inTestMode()) return;
#ifdef ENABLE_ORACLE
            LogTrace(TRACE3) << __func__ << " tscript ************* savepoint tscript";
            make_curs("rollback to savepoint tscript").exec();
#endif //ENABLE_ORACLE
            DbCpp::mainPgManagedSession(STDLOG).rollbackInTestMode();
            if(auto arxSess = dynamic_cast<DbCpp::PgSession*>(get_arx_pg_rw_sess(STDLOG))) {
                arxSess->rollbackInTestMode();
            }
            PgCpp::rollbackInTestMode();
            if (nosir_mode()) {
                rollbackInTestMode();
                // this commit makes SP_XP_TESTING to prevent core dump in nosir mode
                commit();
            }

            TsCallbacks::afterTest();
        }
    private:
        xp_testing::TlgAccumulator tlgAccumulator_;
    };

    TsCallbacksImpl _TsCallbacks;

}} /* namespace xp_testing::tscript */

#endif /* XP_TESTING */

