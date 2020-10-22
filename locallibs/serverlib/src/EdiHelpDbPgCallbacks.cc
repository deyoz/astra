#ifdef ENABLE_PG
#include "serverlib/EdiHelpDbPgCallbacks.h"
#include "pg_cursctl.h"
#include "serverlib/internal_msgid.h"
#include "serverlib/testmode.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "serverlib/dates.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace ServerFramework
{

class PgEdiHelpSelector
{
public:
    PgEdiHelpSelector(EdiHelp &ediHelp);

    void setCondition(const std::string &condition)
    {
        m_condition = condition;
    }

    std::string query()
    {
        return (m_selectText + m_condition);
    }
    PgCpp::CursCtl& def(PgCpp::CursCtl &cur);
    int fen(PgCpp::CursCtl &cur);
    int EXfet(PgCpp::CursCtl &cur);
    PgCpp::CursCtl make_cursor(PgCpp::SessionDescriptor sd);
private:
    EdiHelp &m_ediHelp;
    std::string m_selectText;
    std::string m_condition;
    std::string m_int_msgid_str;
};

PgEdiHelpSelector::PgEdiHelpSelector(EdiHelp &ediHelp)
    : m_ediHelp(ediHelp)
{
    m_selectText = "SELECT "
                   "INTMSGID, INSTANCE, ADDRESS, "
                   "TEXT, TIMEOUT, PULT, SESSION_ID "
                   "FROM EDI_HELP ";
}

int PgEdiHelpSelector::fen(PgCpp::CursCtl &cur)
{
    int res = cur.fen();
    if(!res) {
        m_ediHelp.id = InternalMsgId::fromString(m_int_msgid_str).id();
    }
    return res;
}

int PgEdiHelpSelector::EXfet(PgCpp::CursCtl &cur)
{
    int res = cur.EXfet();
    if(!res) {
        m_ediHelp.id = InternalMsgId::fromString(m_int_msgid_str).id();
    }
    return res;
}

PgCpp::CursCtl& PgEdiHelpSelector::def(PgCpp::CursCtl &cur)
{
    return cur
        .autoNull()
        .def(m_int_msgid_str)
        .def(m_ediHelp.instance)
        .def(m_ediHelp.address)
        .def(m_ediHelp.txt)
        .def(m_ediHelp.timeout)
        .def(m_ediHelp.pult)
        .defNull(m_ediHelp.session_id, 0);
}

PgCpp::CursCtl PgEdiHelpSelector::make_cursor(PgCpp::SessionDescriptor sd)
{
    PgCpp::CursCtl cur = make_pg_curs(sd, query());
    def(cur);
    return cur;
}

static std::list<EdiHelp> select_all_(PgCpp::SessionDescriptor sd,
                                      const char *pult, bool for_update)
{
    EdiHelp ediHelp;
    PgEdiHelpSelector selector(ediHelp);
    std::string cond = "WHERE PULT = :pult ";
    if(for_update)
        cond += "for update";
    selector.setCondition(cond);
    PgCpp::CursCtl cur = make_pg_curs(sd, selector.query().c_str());
    cur
        .bind(":pult", pult);
    selector.def(cur);
    cur.exec();

    std::list<EdiHelp> result;

    while (!selector.fen(cur)) {
        result.push_back(ediHelp);
    }

    return result;
}

static std::list<EdiHelp> select_all_(PgCpp::SessionDescriptor sd,
                                      const InternalMsgId &msgid, bool for_update)
{
    EdiHelp ediHelp;
    PgEdiHelpSelector selector(ediHelp);
    std::string cond = "where INTMSGID = :intmsgid order by TIMEOUT ";
    if(for_update)
        cond += "for update ";
    selector.setCondition(cond);
    PgCpp::CursCtl cur = make_pg_curs(sd, selector.query().c_str());
    cur
            .stb()
            .bind(":intmsgid", msgid.asString());
    selector.def(cur);
    cur.exec();

    std::list<EdiHelp> result;

    while (!selector.fen(cur)) {
        result.push_back(ediHelp);
    }

    return result;
}


static void delete_edihelp(PgCpp::SessionDescriptor sd, const char *pult, int session_id)
{
    auto cur = make_pg_curs(sd, "delete from edi_help where pult = :pult and session_id = :session_id");
    cur.bind(":pult", pult).bind(":session_id", session_id).exec();
    if(cur.rowcount() != 1)
        LogError(STDLOG) << cur.rowcount() << " rows deleted for pult: " << pult << " session_id: " << session_id;
}

static void delete_edihelp(PgCpp::SessionDescriptor sd, const char *pult, const InternalMsgId &msgid)
{
    auto cur = make_pg_curs(sd, "delete from edi_help "
                                "where ctid in ( "
                                    "select ctid from edi_help "
                                    "where pult = :pult "
                                    "and session_id = 0 "
                                    "and intmsgid = :intmsgid "
                                    "order by timeout "
                                    "limit 1)");
    cur.bind(":pult", pult).bind(":intmsgid", msgid.asString()).exec();
    if(cur.rowcount() != 1)
        LogError(STDLOG) << cur.rowcount() << " rows deleted for pult: " << pult << " msgid: " << msgid.asString();
}

static void delete_edihelp(PgCpp::SessionDescriptor sd, const InternalMsgId &msgid, int session_id)
{
    auto cur = make_pg_curs(sd, "delete from edi_help where intmsgid = :intmsgid and session_id = :session_id");
    cur.stb()
       .bind(":intmsgid", msgid.asString())
       .bind(":session_id", session_id)
       .exec();
    if(cur.rowcount() != 1)
        LogError(STDLOG) << cur.rowcount() << " rows deleted for intmsgid: " << msgid.asString() << " session_id: " << session_id;
}

static unsigned leftover_edihelp(PgCpp::SessionDescriptor sd, const InternalMsgId &msgid)
{
    unsigned count = 0;
    auto cur = make_pg_curs(sd, "SELECT COUNT(*) FROM EDI_HELP WHERE INTMSGID = :intmsgid");
    cur.bind(":intmsgid", msgid.asString()).def(count).EXfet();
    return count;
}

boost::optional<ConfirmInfo> EdiHelpDbPgCallbacks::confirm_notify_oraside(const char *pult, int session_id) const
{
    LogTrace(TRACE3) << __FUNCTION__ << '(' << pult << ", " << session_id << ')';

    const auto ledihelp = select_all_(sd, pult, true /* for update */);
    boost::optional<EdiHelp> found;
    ConfirmInfo ci = {};

    for(auto &&eh: ledihelp)
    {
        if(eh.session_id > 0) {
            if(eh.session_id == session_id) {
                found = eh;
                delete_edihelp(sd, pult, session_id);
                break;
            }
        } else if (eh.session_id == 0) {
            found = eh;
            delete_edihelp(sd, pult, InternalMsgId(eh.id));
            break;
        }
    }
    if(found) {
        ci.leftover = leftover_edihelp(sd, InternalMsgId(found->id));
        ci.instanceName = found->instance;
        ci.signalTxt = found->txt;
        ci.address = found->address;
        ci.id = found->id;
        return ci;
    } else {
        return boost::none;
    }
}

boost::optional<ConfirmInfo> EdiHelpDbPgCallbacks::confirm_notify_oraside(
        const InternalMsgId& msgid, int session_id, const std::string &instance_name) const
{
    LogTrace(TRACE3) << __FUNCTION__ << '(' << msgid << ", " << session_id << ')';

    const auto ledihelp = select_all_(sd, msgid, true /* for update */);
    boost::optional<EdiHelp> found;
    ConfirmInfo ci = {};

    for(auto &&eh: ledihelp)
    {
        if(eh.session_id == session_id) {
            found = eh;
            delete_edihelp(sd, msgid, session_id);
            break;
        }
    }
    if(found) {
        ci.leftover = leftover_edihelp(sd, InternalMsgId(found->id));
        ci.instanceName = found->instance;
        ci.signalTxt = found->txt;
        ci.address = found->address;
        ci.id = found->id;
        return ci;
    } else {
        return boost::none;
    }
}

boost::optional<EdiHelp> EdiHelpDbPgCallbacks::select_one(const InternalMsgId& msgid, int session_id) const
{
    EdiHelp result;
    PgEdiHelpSelector selector(result);
    selector.setCondition("WHERE INTMSGID = :intmsgid "
                          "AND SESSION_ID = :session_id ");
    auto cur = selector.make_cursor(sd);
    cur.stb()
       .bind(":intmsgid", msgid.asString())
       .bind(":session_id", session_id);

    selector.EXfet(cur);

    if (cur.err() == PgCpp::NoDataFound) {
        LogTrace(TRACE1) << "EdiHelp not found for msgid: " << msgid.asString() << ", session_id: " << session_id;
        return boost::optional<EdiHelp>();
    }

    return result;
}

void EdiHelpDbPgCallbacks::create_db(const EdiHelp &eh, bool clear_other_intmsgid, bool autonomous) const
{
    LogTrace(TRACE5) << __func__ << "(clear_other_intmsgid="<<clear_other_intmsgid<<", autonomous="<<autonomous<<") "
                     << InternalMsgId(eh.id).asString();

    if(clear_other_intmsgid) {
        auto delcur = make_pg_curs(sd, "DELETE FROM EDI_HELP WHERE PULT = :pult AND INSTANCE = :instance AND INTMSGID != :id");

        delcur
            .bind(":pult", eh.pult)
            .bind(":instance", eh.instance)
            .bind(":id", InternalMsgId(eh.id).asString())
            .exec();
        if (delcur.rowcount())
            LogWarning(STDLOG) << delcur.rowcount() << " rows deleted for pult " << eh.pult
                               << " msgid: " << InternalMsgId(eh.id);

    }

    auto cur = make_pg_curs(sd,
                "INSERT INTO EDI_HELP "
                " (PULT, INTMSGID, INSTANCE, ADDRESS, TEXT, DATE1, SESSION_ID, TIMEOUT) "
                "VALUES "
                " (:pult, :id, :instance, :addr, :txt, :local_time, :sid, :timeout)");

    boost::posix_time::ptime local_time = boost::posix_time::second_clock::local_time();
    cur
        .bind(":pult", eh.pult)
        .bind(":instance", eh.instance)
        .bind(":id", InternalMsgId(eh.id).asString())
        .bind(":local_time", local_time)
        .bind(":timeout",    eh.timeout)
        .bind(":addr",       eh.address)
        .bind(":txt",        eh.txt)
        .bind(":sid",        eh.session_id)
        .exec();
}

void EdiHelpDbPgCallbacks::clear_old_records() const
{
    LogTrace(TRACE3) << __FUNCTION__;
    const Dates::ptime amin_ago = Dates::second_clock::local_time() - Dates::seconds(60);
    make_pg_curs(sd, "delete from edi_help where timeout < :min_ago")
            .bind(":min_ago", amin_ago)
            .exec();

}

std::list<EdiHelp> EdiHelpDbPgCallbacks::select_all(const char *pult) const
{
    return select_all_(sd, pult, false/*for update*/);
}

std::list<EdiHelp> EdiHelpDbPgCallbacks::select_all(const InternalMsgId& msgid) const
{
    EdiHelp ediHelp;
    PgEdiHelpSelector selector(ediHelp);
    selector.setCondition("WHERE INTMSGID = :intmsgid ");
    auto cur = selector.make_cursor(sd);
    cur.bind(":intmsgid", msgid.asString()).exec();

    std::list<EdiHelp> result;

    while (!selector.fen(cur)) {
        result.push_back(ediHelp);
    }
    return result;
}

} // namespace ServerFramework
#endif//ENABLE_PG
