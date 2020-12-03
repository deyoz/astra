#include <array>
#if HAVE_CONFIG_H
#endif

#include <iomanip>
#include <tcl.h>
#include "EdiHelpManager.h"
#include "cursctl.h"
#include "query_runner.h"
#include "posthooks.h"
#include "ehelpsig.h"
#include "timer.h"
#include "EdiHelpDbCallbacks.h"
#include "dates_oci.h"
#include "std_array_oci.h"
#include "timer.h"

#include "tcl_utils.h"
#include "internal_msgid.h"
#include "msg_const.h"

#include <boost/optional.hpp>
#include "testmode.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"
#include "tscript.h"
#ifdef XP_TESTING
#include "checkunit.h"
#endif // XP_TESTING

using namespace boost::posix_time;

namespace ServerFramework
{
using namespace OciCpp;
using namespace std;

#ifdef XP_TESTING
static boost::optional<std::string> &redisplay_holder()
{
    static boost::optional<std::string> holder;
    return holder;
}

void clearRedisplay()
{
    redisplay_holder().reset();
}
void listenRedisplay()
{
    redisplay_holder().reset(std::string());
}

std::string getRedisplay()
{
    return redisplay_holder() ? redisplay_holder().get() : "";
}

/**
  * store redisplay message. internal use only.
 */
void setRedisplay(const std::string& redisp)
{
    if(redisplay_holder()) {
        if(!getRedisplay().empty()) {
            std::string err = "trying to set redisplay message ";
            err += redisp;
            err += " while this one already present: ";
            err += getRedisplay();
            throw Exception(STDLOG, __FUNCTION__, err);
        }
        redisplay_holder().reset(redisp);
    }
}
#endif /*XP_TESTING*/

std::ostream& operator << (std::ostream& ostream, const EdiHelp &eh)
{
    return ostream << eh.pult
             << " " << eh.address
             << " " << eh.instance
             << " sessionId: " << eh.session_id
             << " msgid: " << InternalMsgId(eh.id)
             << " to: " << eh.timeout
             << " txt: " << eh.txt;
}

void EdiHelpManager::setDebug()
{
    saved_flag|=MSG_DEBUG;
}

void EdiHelpManager::removeMustWaitFlag()
{
    saved_flag&=~MSG_ANSW_STORE_WAIT_SIG;

    // ¢ë§®¢ set_msg_type_and_timeout ¨§¬¥­ï¥â áâ â¨ç¥áª¨¥ ¯¥à¥¬¥­­ë¥ saved_flag ¨ saved_timeout ¢ sirena_queue.cc
    // ®­ ­¥®¡å®¤¨¬, çâ®¡ë ¨§¡¥¦ âì ®¦¨¤ ­¨ï ¯® â ©¬ ãâã ¢ á«ãç ¥ ¨áª«îç¥­¨ï ¯®á«¥ ¢ë§®¢  configForPerespros
    set_msg_type_and_timeout(saved_flag, 0);
}

void EdiHelpManager::configForPerespros(const char *nick, const char *file, int line,
                                        const char * requestText, int session_id, int timeout)
{
    ProgTrace(TRACE1, "%s(%s:%i  session_id=%i, timeout=%i, %s)", __func__, file, line, session_id, timeout, requestText);
    if(saved_flag & flagPerespros) {
        timeout = old_timeout > timeout ? old_timeout : timeout;
    }

    timeout = timeout && timeout < minimum_timeout ? minimum_timeout : timeout;
    if(timeout > max_timeout)
        max_timeout = timeout;
    old_timeout = timeout;
    saved_flag |= flagPerespros;

    if(saved_flag) {
        set_msg_type_and_timeout(saved_flag,timeout);

        static const char *addr;
        if (addr == 0) {
            if(inTestMode()) {
                addr = strdup("testmode/socket");
            } else {
                auto obj = Tcl_ObjGetVar2(getTclInterpretator(),
                                     current_group(),
                                     Tcl_NewStringObj("SIGNAL", -1),
                                     TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
                if (!obj)
                {
                    ProgError(STDLOG, "Tcl_ObjGetVar2:%s",
                              Tcl_GetString(Tcl_GetObjResult(getTclInterpretator())));
                    Tcl_ResetResult(getTclInterpretator());
                }
                addr = strdup(Tcl_GetString(obj));
            }
        }

        if(timeout) {
            EdiHelp eh;
            eh.id = my_query_runner->getEdiHelpManager().msgId().id();
            strcpy(eh.instance, ServerFramework::EdiHelpManager::instanceName().c_str());
            eh.pult = my_query_runner->pult();
            eh.txt = requestText;
            eh.session_id = session_id;
            ptime local_time = second_clock::local_time();
            eh.timeout = local_time + seconds(timeout - timeout_difference);
            strcpy(eh.address, addr);
            LogTrace(TRACE1) << __FUNCTION__ << ": " << eh << "   --------------------------------";
            EdiHelpDbCallbacks::instance()->create_db(eh, !one_pult_many_msgid);
        }
    }
}

void EdiHelpManager::cleanOldRecords()
{
    EdiHelpDbCallbacks::instance()->clear_old_records();
}

const std::string &EdiHelpManager::instanceName()
{
    static const std::string instance_name = readStringFromTcl("INSTANCE_NAME", "NIN").substr(0,3);
    return instance_name;
}

bool EdiHelpManager::copyEdiHelpWithNewEdisession(const InternalMsgId& intmsgid, int session_id, int new_session_id)
{
    LogTrace(TRACE3) << __FUNCTION__ << "  " << intmsgid;
    if(auto ediHelp = EdiHelpDbCallbacks::instance()->select_one(intmsgid, session_id))
    {
        ediHelp->session_id = new_session_id;
        EdiHelpDbCallbacks::instance()->create_db(*ediHelp,
                                                  false /* clear other intmsgids */,
                                                  true /* autonomous transaction */);
        return true;
    }
    tst();
    return false;
}

static void dumpEdihelp(const char *pult)
{
    LogTrace(TRACE1) << __func__ << '(' << pult << ')';
    std::list<EdiHelp> lEdiHelp = EdiHelpDbCallbacks::instance()->select_all(pult);
    for(const EdiHelp &eh:  lEdiHelp) {
        LogTrace(TRACE1) << "  " << eh;
    }
    if(lEdiHelp.empty()) {
        LogTrace(TRACE1) << "  No EDI_HELP found for pult " << pult;
    }
}

static void dumpEdihelp(const InternalMsgId& msgid)
{
    LogTrace(TRACE1) << __func__ << '(' << msgid << ')';
    for(auto&& eh : EdiHelpDbCallbacks::instance()->select_all(msgid))
        LogTrace(TRACE1) << "  " << eh;
}

static void confirm_notify_oraside(const boost::optional<ConfirmInfo> &ci,
                                   const std::string &key, int session_id)
{
    if(not ci) {
        LogWarning(STDLOG) << "No records in EDI_HELP for pult/msgid: " << key
                           << " edisession_id: " << session_id;
    } else if(ci->leftover == 0) {
        LogTrace(TRACE1) << "prepare signal: " << ci->signalTxt;
#ifdef XP_TESTING
        if(inTestMode()) {
            setRedisplay(ci->signalTxt);
        }
#endif /* XP_TESTING */
        if(ci->instanceName != EdiHelpManager::instanceName()) {
            LogError(STDLOG) << "Trying to send signal to another application instance."
                                " Local : " << EdiHelpManager::instanceName() <<
                                " Remote: " << ci->instanceName <<
                                " Signal: " << ci->signalTxt;
        } else {
            Posthooks::sethAfter(EdiHelpSignal(InternalMsgId(ci->id), ci->address.c_str(), ci->signalTxt.c_str()));
        }
    } else {
        LogTrace(TRACE1) << "more records (" << ci->leftover << ") for '" << key << "'";
    }
}

static void confirm_notify_oraside(const char *pult, int session_id)
{
    HelpCpp::Timer timer;

    LogTrace(TRACE1) << __FUNCTION__ << ": pult = " << pult << ", instance = " << ServerFramework::EdiHelpManager::instanceName();

    const auto ci = EdiHelpDbCallbacks::instance()->confirm_notify_oraside(pult, session_id);
    if(not ci or ci->leftover > 1)
        dumpEdihelp(pult);

    confirm_notify_oraside(ci, pult, session_id);

    LogTrace(TRACE1) << __FUNCTION__ << ": " << timer;
}

static void confirm_notify_oraside(const InternalMsgId& msgid, int session_id)
{
    HelpCpp::Timer timer;

    const auto ci = EdiHelpDbCallbacks::instance()->confirm_notify_oraside(msgid, session_id, EdiHelpManager::instanceName());
    if(not ci or ci->leftover > 1)
        dumpEdihelp(msgid);

    confirm_notify_oraside(ci, msgid.asString(), session_id);

    LogTrace(TRACE1) << __FUNCTION__ << ": " << timer;
}

#ifdef XP_TESTING
void imitate_confirm_notify_oraside_for_bloody_httpsrv(const ServerFramework::InternalMsgId& msgid, const std::string& signal)
{
    if(not inTestMode()) {
        return;
    }
    OciCpp::CursCtl c("delete from edi_help where intmsgid=:id and instance=:ins and text=:txt and rownum<2", STDLOG);
    c.bind(":id", msgid.id()).bind(":ins", ServerFramework::EdiHelpManager::instanceName()).bind(":txt", signal)
     .exec();
    if(c.rowcount() != 1)
    {
        dumpEdihelp(msgid);
        if(xp_testing::tscript::nosir_mode())
            throw ServerFramework::Exception("no rows deleted from edi_help...");
        fail_if(true, "no rows deleted from edi_help for [%u %u %u]@%s and txt <%s>",
                      msgid.id(0), msgid.id(1), msgid.id(2),
                      ServerFramework::EdiHelpManager::instanceName().c_str(),
                      signal.c_str());
    }
    int flag = 0;
    const int e = OciCpp::CursCtl("select 1 from edi_help where intmsgid=:id and instance=:ins and rownum<2", STDLOG)
        .bind(":id", msgid.id()).bind(":ins", ServerFramework::EdiHelpManager::instanceName())
        .def(flag)
        .EXfet();

    if(xp_testing::tscript::nosir_mode()) {
        if (e != NO_DATA_FOUND or flag != 0) {
            throw ServerFramework::Exception("records left in edi_help...");
        }
    } else {
        ck_assert_int_eq(e, NO_DATA_FOUND);
        ck_assert_int_eq(flag, 0);
    }
}
#endif // XP_TESTING

void EdiHelpManager::confirm_notify(const char *pult, int session_id)
{
    confirm_notify_oraside(pult, session_id);
}

void EdiHelpManager::confirm_notify(const InternalMsgId& msgid, int session_id)
{
    confirm_notify_oraside(msgid, session_id);
}

bool EdiHelpManager::mustWait() const
{
    return willSuspend(saved_flag);
}

InternalMsgId EdiHelpManager::msgId() const
{
    std::array<uint32_t,3> id;
    std::copy(get_internal_msgid(), get_internal_msgid()+3, id.begin());
    return InternalMsgId(id);
}

} // namespace ServerFramework

#include "checkunit.h"
#include "query_runner.h"
#include "sirenaproc.h"
#ifdef XP_TESTING
START_TEST(int_msg_id)
{
    using namespace ServerFramework;
    std::array<uint32_t,3> msgid = {{0x43, 0xffffffff, 0xfe}};
    InternalMsgId msg(msgid);

    LogTrace(TRACE4) << "msgid str: " << msg.asString();
    fail_unless(msg.asString() == "00000043ffffffff000000fe", "<%s> instead of <00000043ffffffff000000fe>", msg.asString().c_str());
    fail_unless(msg.id()[0] == 0x43, "inv msg id");
    fail_unless(msg.id()[1] == 0xffffffff, "inv msg id");
    fail_unless(msg.id()[2] == 0xfe, "inv msg id");

    std::array<uint32_t,3> msgid2 = {{0x4323,0x00,0xfe}};
    InternalMsgId msg2(msgid2);
    LogTrace(TRACE4) << "msgid str: " << msg2.asString();
    fail_unless(msg2.asString() == "0000432300000000000000fe", "inv string");
}
END_TEST

void confirm_notify_oraside_tst()
{
    using namespace ServerFramework;
    QueryRunner qr(std::make_shared<EdiHelpManager>(MSG_ANSW_STORE_WAIT_SIG));
    qr.setPult("Œ‚Œ");
    qr.getEdiHelpManager().configForPerespros(STDLOG, "LALA###", 1010, 20);
    dumpEdihelp("Œ‚Œ");

    confirm_notify_oraside("Œ‚Œ", 1010);
}

void confirm_notify_oraside_long_msg_test_tst()
{
    using namespace ServerFramework;
    static const auto msgtext = "ZH/BUYTICKET/FORWARD=\"FROM=2000002/TO=2010290/DAY=22/MONTH=09/TIME=21:05/TRAIN=126Ÿ/CARRIER=’Š‘/N_CAR=18/TYPE_CAR=Š/SERVICE_CLASS=2‹/N_UP=2/N_DOWN=2/DIAPASON=1-15/SEX=‘/IN_ONE_KUPE=1/REMOTECHECKIN=1\"/BACKWARD=\"FROM=2010290/TO=2000002/DAY=27/MONTH=09/TIME=22:55/TRAIN=126—/CARRIER=’Š‘/N_CAR=18/TYPE_CAR=Š/SERVICE_CLASS=2‹/N_UP=2/N_DOWN=2/DIAPASON=1-15/SEX=‘/REMOTECHECKIN=1\"/ISSUBURBANTRAIN=0/DIRECTIONGROUP=0/RZHDPROVIDER=GRAND/PHONE=+79174036096/EMAIL=ELVIRAAVIA@MAIL.RU/FOP=/ISRESERVATION=0/ADULT_DOC=\"8010036445/Œ€Š€‚=‚‹€„ˆ‘‹€‚=€‹…Š‘€„‚ˆ—/17021965/RUS/Œ/[-]/////1/1////ELVIRAAVIA@MAIL.RU/+79174036096\"/ADULT_DOC=\"8005175298/•€Œ‚€=‹ˆŸ=…’‚€/18061976/RUS/F/[-]/////1/1////ELVIRAAVIA@MAIL.RU/+79174036096\"/ADULT_DOC=\"8012731497/“’€Š‚€=ƒ€‹ˆ€=œ…‚€/07031968/RUS/F/[-]/////1/1////ELVIRAAVIA@MAIL.RU/+79174036096\"/ADULT_DOC=\"8018858952/ƒ€‰“‹‹ˆ€=…‹…€=‚ˆŠ’‚€/24091973/RUS/F/[-]/////1/1////ELVIRAAVIA@MAIL.RU/+79174036096\"/FLAGS=63/STAN=7T78DD/STAN2=7T78DK/MULTISTAGE=GRANDasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdasdaasdasd"; // 1300 chars
    QueryRunner qr(std::make_shared<EdiHelpManager>(MSG_ANSW_STORE_WAIT_SIG));
    qr.setPult("Œ‚Œ");
    qr.getEdiHelpManager().configForPerespros(STDLOG, msgtext, 1010, 20);
    dumpEdihelp("Œ‚Œ");
    confirm_notify_oraside("Œ‚Œ", 1010);
}

#define SUITENAME "Serverlib"
TCASEREGISTER(0,0)
{
    ADD_TEST(int_msg_id);
}TCASEFINISH
#endif /*XP_TESTING*/
