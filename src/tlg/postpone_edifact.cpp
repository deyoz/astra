#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "postpone_edifact.h"
#include "edi_handler.h" // TODO
#include "remote_system_context.h" // TODO

#include "tlg.h"

#include <serverlib/cursctl.h>
#include <serverlib/dates_oci.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/testmode.h>

#include <boost/lexical_cast.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

void PostponeEdiHandling::insertDb(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId)
{
    OciCpp::CursCtl cur = make_curs("insert into POSTPONED_TLG(MSG_ID, EDISESS_ID) "
                                    "values(:msg, :edisess)");

    cur.bind(":msg", tnum.num)
       .bind(":edisess", sessId)
       .exec();

    LogTrace(TRACE1) << "insert into POSTPONED_TLG for edisess=" << sessId << "; msg_id=" << tnum;
}

tlgnum_t PostponeEdiHandling::deleteDb(edilib::EdiSessionId_t sessId)
{
    char tnum[tlgnum_t::TLG_NUM_LENGTH + 1] = {};
    OciCpp::CursCtl cur = make_curs(
            "begin\n"
            ":msg:=NULL;\n"
            "delete from POSTPONED_TLG where EDISESS_ID=:edisess "
            "returning MSG_ID into :msg; \n"
            "end;");

    cur.bind(":edisess", sessId)
       .bindOutNull(":msg", tnum, "")
       .exec();

    LogTrace(TRACE1) << "delete from POSTPONED_TLG for edisess=" << sessId
                     << "; got msg_id=" << tnum;

    return tlgnum_t(tnum);
}

void PostponeEdiHandling::addToQueue(const tlgnum_t& tnum)
{
    /*OciCpp::CursCtl cur = make_curs(
"INSERT INTO tlg_queue(id, sender, tlg_num, receiver, type, priority, status, time, ttl, time_msec, last_send) "
"VALUES(:id, :sender, :tlg_num, :receiver, :type, 1, 'PUT', :time, :ttl, :time_msec, NULL)");

    cur.bind(":id", tnum.num)
       .bind(":sender", "LOOPB")
       .bind(":tlg_num", tnum.num)
       .bind(":receiver", "LOOPB")
       .bind(":type", "INA")
       .bind(":time", Dates::currentDate())
       .bind(":ttl", 10)
       .bind(":time_msec", 0)
       .exec();*/


    std::string text = getTlgText2(tnum);

    tlg_info tlgi = {};
    tlgi.id = boost::lexical_cast<int>(tnum.num);
    tlgi.sender = "LOOPB";
    tlgi.text = text;

#ifdef XP_TESTING
    if(inTestMode()) {
        Ticketing::RemoteSystemContext::SystemContext::free();
        handle_edi_tlg(tlgi);
    }
#endif//XP_TESTING
}

void PostponeEdiHandling::postpone(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId)
{
    insertDb(tnum, sessId);
    updateTlgToPostponed(tnum);
}

void PostponeEdiHandling::postpone(int tnum, edilib::EdiSessionId_t sessId)
{
    tlgnum_t tlgNum(boost::lexical_cast<std::string>(tnum));
    postpone(tlgNum, sessId);
}

tlgnum_t PostponeEdiHandling::deleteWaiting(edilib::EdiSessionId_t sessId)
{
    LogTrace(TRACE3) << "try to find postponed tlg for session: " << sessId;
    tlgnum_t tnum = deleteDb(sessId);
    if(tnum.num.valid())
    {
        LogTrace(TRACE1) << "putTlg2InputQueue postponed tlg with num: " << tnum;
        addToQueue(tnum);
    }
    return tnum;
}

tlgnum_t PostponeEdiHandling::findPostponeTlg(edilib::EdiSessionId_t sessId)
{
    char tnum[tlgnum_t::TLG_NUM_LENGTH + 1] = {};
    OciCpp::CursCtl cur = make_curs(
"select MSG_ID from POSTPONED_TLG where EDISESS_ID=:edisess");
    cur.bind(":edisess", sessId)
       .def(tnum)
       .EXfet();
    if(cur.err() != NO_DATA_FOUND) {
        tst();
        return tlgnum_t(tnum);
    }

    return tlgnum_t();
}

//---------------------------------------------------------------------------------------

void updateTlgToPostponed(const tlgnum_t& tnum)
{
    OciCpp::CursCtl cur = make_curs(
"update TLGS set POSTPONED=1 where ID=:msg_id");
    cur.bind(":msg_id", tnum.num)
       .exec();
}

bool isTlgPostponed(const tlgnum_t& tnum)
{
    bool postponed = false;
    OciCpp::CursCtl cur = make_curs(
"select POSTPONED from TLGS where ID=:msg_id");
    cur.bind(":msg_id", tnum.num)
       .defNull(postponed, false)
       .EXfet();
    if(cur.err() != NO_DATA_FOUND) {
        return postponed;
    }

    return false;
}

}//namespace TlgHandling
