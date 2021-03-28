#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "postpone_edifact.h"
#include "edi_handler.h"
#include "remote_system_context.h"
#include "tlg_source_edifact.h"
#include "tlg.h"

#include <serverlib/cursctl.h>
#include <serverlib/dates_oci.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/testmode.h>
#include <serverlib/rip_oci.h>
#include <edilib/edi_user_func.h>
#include <edilib/edi_tables.h>
#include <serverlib/dbcpp_cursctl.h>

#include "PgOraConfig.h"

#include <boost/optional/optional_io.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

const tlgnum_t TlgToBePostponed::tlgNum() const
{
    return m_tlgNum;
}

int TlgToBePostponed::tlgnum() const
{
    return std::stoi(m_tlgNum.num.get());
}

//-----------------------------------------------------------------------------

void PostponeEdiHandling::insertDb(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId)
{
    LogTrace(TRACE3) << "add session " << sessId << " for postpone tlg " << tnum;
    auto cur = make_db_curs(
"insert into POSTPONED_TLG(MSG_ID, EDISESS_ID) "
"values(:msg, :edisess)",
                PgOra::getRWSession("POSTPONED_TLG"));

    cur
            .bind(":msg", tnum.num.get())
            .bind(":edisess", sessId.get())
            .exec();

    LogTrace(TRACE1) << "insert into POSTPONED_TLG for edisess=" << sessId << "; msg_id=" << tnum;
}

static boost::optional<tlgnum_t> deleteDbOra(edilib::EdiSessionId_t sessId)
{
    char tnum[telegrams::TLG_NUM_LENGTH + 1] = {};
    auto cur = make_curs(
"begin\n"
":msg:=NULL;\n"
"delete from POSTPONED_TLG where EDISESS_ID=:edisess "
"returning MSG_ID into :msg; \n"
"select count(*) into :remained from POSTPONED_TLG "
"where MSG_ID=:msg; \n "
"end;");

    int remained = 0;
    cur
            .bind(":edisess", sessId)
            .bindOutNull(":msg", tnum, "")
            .bindOutNull(":remained", remained, -1)
            .exec();

    std::string tmpNum(tnum);

    if(tmpNum == "") {
        tst();
        return boost::none;
    }

    LogTrace(TRACE3) << "Remained " << remained << " sessions for postpone tlg " << tnum;

    if(remained == 0) {
        LogTrace(TRACE1) << "delete from POSTPONED_TLG for edisess=" << sessId
                         << "; got msg_id=" << tnum;

        if(!tmpNum.empty())
            return tlgnum_t(tnum);
    }

    return boost::none;
}

static boost::optional<tlgnum_t> deleteDbPg(edilib::EdiSessionId_t sessId)
{
    auto cur = make_db_curs(
"delete from POSTPONED_TLG where EDISESS_ID = :sess_id "
"returning MSG_ID",
                PgOra::getRWSession("POSTPONED_TLG"));

    tlgnum_t::num_t::base_type msg_id;
    cur
            .def(msg_id)
            .bind(":sess_id", sessId.get())
            .exfet();

    if(cur.err() != DbCpp::ResultCode::NoDataFound) {
        auto cur = make_db_curs(
"select count(*) from POSTPONED_TLG where MSG_ID = :msg_id",
                    PgOra::getRWSession("POSTPONED_TLG"));

        int remained = 0;
        cur
                .def(remained)
                .bind(":msg_id", msg_id)
                .EXfet();

        if(remained == 0) {
            LogTrace(TRACE1) << "delete from POSTPONED_TLG for edisess=" << sessId << "; "
                             << "got msg_id=" << msg_id;
            return tlgnum_t(msg_id);
        }
    }

    return {};
}

boost::optional<tlgnum_t> PostponeEdiHandling::deleteDb(edilib::EdiSessionId_t sessId)
{
    if(PgOra::supportsPg("POSTPONED_TLG")) {
        return deleteDbPg(sessId);
    } else {
        return deleteDbOra(sessId);
    }
}

void sendCmdToHandler(const std::string& ediText)
{
    if(ReadEdiMessage(ediText.c_str()) == EDI_MES_OK) {
        _EDI_REAL_MES_STRUCT_ *pMes = GetEdiMesStruct();
        switch(pMes->pTempMes->Type.type)
        {
        case DCQCKI:
        case DCQCKU:
        case DCQCKX:
        case DCQPLF:
        case DCQBPR:
        case DCQSMF:
        {
            sendCmd("CMD_ITCI_REQ_HANDLER","H");
            break;
        }
        case DCRCKA:
        case DCRSMF:
        {
            sendCmd("CMD_ITCI_RES_HANDLER","H");
            break;
        }
        case CUSRES:
        case CUSUMS:
        {
            sendCmd("CMD_IAPI_EDI_HANDLER","H");
            break;
        }
        default:
            sendCmd("CMD_EDI_HANDLER","H");
            break;
        }
    }
}

void PostponeEdiHandling::addToQueue(const tlgnum_t& tnum)
{
    LogTrace(TRACE1) << "putTlg2InputQueue postponed tlg with num: " << tnum;

    TlgHandling::TlgSourceEdifact tlg = TlgSource::readFromDb(tnum);

    TEdiTlgSubtype tlgSubtype = specifyEdiTlgSubtype(tlg.text());

    DbCpp::CursCtl cur = make_db_curs(
"insert into TLG_QUEUE(ID, SENDER, TLG_NUM, RECEIVER, TYPE, SUBTYPE, PRIORITY, STATUS, TIME, TTL, TIME_MSEC) "
"values(:id, :sender, :tlg_num, :receiver, 'INA', :subtype, 1, 'PUT', :time, 10, 0)",
        PgOra::getRWSession("TLG_QUEUE"));

    cur
            .stb()
            .bind(":id", std::stoull(tnum.num.get()))
            .bind(":sender", tlg.fromRot())
            .bind(":tlg_num", tlg.gatewayNum())
            .bind(":receiver", tlg.toRot())
            .bind(":subtype", getEdiTlgSubtypeName(tlgSubtype))
            .bind(":time", Dates::currentDate())
            .exec();

#ifdef XP_TESTING
    if(inTestMode())
    {
        //Ticketing::RemoteSystemContext::SystemContext::free();

        tlg_info tlgi = {};
        tlgi.id = std::stoi(tlg.tlgNum()->num.get());
        tlgi.sender = tlg.fromRot();
        tlgi.text = tlg.text();
        handle_edi_tlg(tlgi);
    }
#endif//XP_TESTING

    sendCmdEdiHandlerAtHook(tlgSubtype);
}

void PostponeEdiHandling::deleteWaiting(const tlgnum_t& tnum)
{
    LogTrace(TRACE3) << "delete postponed records for tlgnum: " << tnum;
    make_db_curs(
"delete from POSTPONED_TLG where MSG_ID = :msg",
               PgOra::getRWSession("POSTPONED_TLG"))
            .bind(":msg", tnum.num.get())
            .exec();
}

void PostponeEdiHandling::postpone(const tlgnum_t& tnum, edilib::EdiSessionId_t sessId)
{
    insertDb(tnum, sessId);
    updateTlgToPostponed(tnum);
}

void PostponeEdiHandling::postpone(int tnum, edilib::EdiSessionId_t sessId)
{
    tlgnum_t tlgNum(std::to_string(tnum));
    postpone(tlgNum, sessId);
}

boost::optional<tlgnum_t> PostponeEdiHandling::deleteWaiting(edilib::EdiSessionId_t sessId)
{
    LogTrace(TRACE3) << "try to find postponed tlg for session: " << sessId;
    boost::optional<tlgnum_t> tnum = deleteDb(sessId);
    if(tnum) {
        addToQueue(*tnum);
    }
    return tnum;
}

boost::optional<tlgnum_t> PostponeEdiHandling::findPostponeTlg(edilib::EdiSessionId_t sessId)
{
    tlgnum_t::num_t::base_type msg_id;
    auto cur = make_db_curs(
"select MSG_ID from POSTPONED_TLG where EDISESS_ID = :sess_id",
                PgOra::getROSession("POSTPONED_TLG"));
    cur
            .bind(":sess_id", sessId.get())
            .def(msg_id)
            .EXfet();
    if(cur.err() != DbCpp::ResultCode::NoDataFound) {
        return tlgnum_t(msg_id);
    }

    return {};
}

//---------------------------------------------------------------------------------------

void updateTlgToPostponed(const tlgnum_t& tnum)
{
    DbCpp::CursCtl cur = make_db_curs(
"update TLGS set POSTPONED=1 where ID=:msg_id",
        PgOra::getRWSession("TLGS"));
    cur
            .stb()
            .bind(":msg_id", std::stoull(tnum.num.get()))
            .exec();
}

bool isTlgPostponed(const tlgnum_t& tnum)
{
    bool postponed = false;
    DbCpp::CursCtl cur = make_db_curs(
"select POSTPONED from TLGS where ID=:msg_id",
        PgOra::getROSession("TLGS"));
    cur
            .stb()
            .bind(":msg_id", std::stoull(tnum.num.get()))
            .defNull(postponed, false)
            .EXfet();

    if(DbCpp::ResultCode::NoDataFound == cur.err()) {
        return false;
    }

    return postponed;
}

}//namespace TlgHandling
