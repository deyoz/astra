#include "serverlib/EdiHelpDbOraCallbacks.h"
#include "dates_oci.h"
#include "std_array_oci.h"
#include "cursctl.h"
#include "serverlib/internal_msgid.h"
#include "serverlib/testmode.h"
#include "oci_selector_char.h"
#include <boost/date_time/posix_time/posix_time.hpp>

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace ServerFramework
{

class EdiHelpSelector
{
public:
    EdiHelpSelector(EdiHelp &ediHelp);

    void setCondition(const std::string &condition)
    {
        m_condition = condition;
    }

    std::string query()
    {
        return (m_selectText + m_condition);
    }
    OciCpp::CursCtl& def(OciCpp::CursCtl &cur);
    OciCpp::CursCtl make_cursor(const char* n, const char* f, int l);
private:
    EdiHelp &m_ediHelp;
    std::string m_selectText;
    std::string m_condition;
};

EdiHelpSelector::EdiHelpSelector(EdiHelp &ediHelp)
    : m_ediHelp(ediHelp)
{
    m_selectText = "SELECT "
                   "INTMSGID, INSTANCE, ADDRESS, "
                   "TEXT, TIMEOUT, PULT, SESSION_ID "
                   "FROM EDI_HELP ";
}

OciCpp::CursCtl& EdiHelpSelector::def(OciCpp::CursCtl &cur)
{
    return cur
        .autoNull()
        .defFull(&m_ediHelp.id, 12, 0, &m_ediHelp.binlen, SQLT_BIN)
        .def(m_ediHelp.instance)
        .def(m_ediHelp.address)
        .def(m_ediHelp.txt)
        .def(m_ediHelp.timeout)
        .def(m_ediHelp.pult)
        .defNull(m_ediHelp.session_id, 0);
}

OciCpp::CursCtl EdiHelpSelector::make_cursor(const char* n, const char* f, int l)
{
    OciCpp::CursCtl cur(query(), n, f, l);
    def(cur);
    return cur;
}

boost::optional<ConfirmInfo> EdiHelpDbOraCallbacks::confirm_notify_oraside(const char *pult, int session_id) const
{
    LogTrace(TRACE3) << __FUNCTION__ << '(' << pult << ", " << session_id << ')';
    std::string request =
        "DECLARE \n";
    if(!inTestMode())
        request += "   PRAGMA AUTONOMOUS_TRANSACTION;\n";

    request +=
        "   record_found NUMBER(1) := 0; \n"
        "   inloop_found NUMBER(1) := 0; \n"
        "   min_to EDI_HELP.TIMEOUT%TYPE := NULL; \n"
        "   intmsgid_ EDI_HELP.INTMSGID%TYPE := NULL; \n"
        "   CURSOR EDI_HELP_CUR IS \n"
        "        SELECT * FROM EDI_HELP WHERE PULT = :pult FOR UPDATE; \n"
        "BEGIN \n"
        "   FOR EDI_HELP_REC IN EDI_HELP_CUR \n"
        "   LOOP \n"
        "       inloop_found := 0; \n"
        "       IF :session_id <> 0 THEN \n "
        "           IF EDI_HELP_REC.SESSION_ID = :session_id THEN \n"
        "               inloop_found := 1; \n"
        "               DELETE FROM EDI_HELP WHERE CURRENT OF EDI_HELP_CUR; \n"
        "           END IF; \n"

        "       ELSIF EDI_HELP_REC.SESSION_ID = 0 THEN \n"
        "           IF min_to IS NULL OR min_to > EDI_HELP_REC.TIMEOUT THEN \n"
        "               min_to := EDI_HELP_REC.TIMEOUT; \n"
        "           END IF; \n"
        "           inloop_found := 1; \n"
        "       END IF; \n"

        "       IF inloop_found > 0 THEN \n"
        "           record_found  := 1; \n"
        "           :instanceName := EDI_HELP_REC.INSTANCE; \n"
        "           :signalTxt    := EDI_HELP_REC.TEXT; \n"
        "           :address      := EDI_HELP_REC.ADDRESS; \n"
        "           :id           := EDI_HELP_REC.INTMSGID; \n"
        "       END IF; \n"
        "       IF intmsgid_ IS NULL THEN \n"
        "           intmsgid_ := EDI_HELP_REC.INTMSGID; \n"
        "       END IF; \n"
        "   END LOOP; \n "
        "   IF record_found = 0 THEN \n"
        "       RAISE_APPLICATION_ERROR(-20101, 'NO EDI_HELP MATCH FOUND'); \n"
        "   END IF; \n"
        "   IF :session_id = 0 THEN "
        "       DELETE FROM EDI_HELP WHERE PULT = :pult AND SESSION_ID = 0 AND "
        "           TIMEOUT = min_to AND INTMSGID = intmsgid_ AND ROWNUM < 2; \n"
        "   END IF; \n"
        "   SELECT COUNT(*) INTO :leftover FROM EDI_HELP WHERE INTMSGID = intmsgid_; \n";
    if(!inTestMode())
        request += "   COMMIT; \n";

    request += "END; \n";

    ConfirmInfo ci = {};
    unsigned short binlen = 0;
    char signalTxt[1001];
    char instanceName[4];
    char address[41];

    OciCpp::CursCtl cur = make_curs(request);
    cur
        .throwAll()
        .noThrowError(20101)
        .bind(":session_id", session_id)
        .bind(":pult", pult)
        .bindOut(":leftover", ci.leftover)
        .bindOut(":signalTxt", signalTxt)
        .bindOut(":instanceName", instanceName)
        .bindOut(":address", address)
        .bindFull(":id", ci.id.data(), 12, 0, &binlen, SQLT_BIN)
        .exec();

    LogTrace(TRACE3) << "err = " << cur.err();
    if (cur.err() == 20101) {
        return boost::none;
    } else {
        ci.signalTxt = signalTxt;
        ci.instanceName = instanceName;
        ci.address = address;
        return ci;
    }
}

boost::optional<ConfirmInfo> EdiHelpDbOraCallbacks::confirm_notify_oraside(
        const InternalMsgId& msgid, int session_id, const std::string &instance_name) const
{
    LogTrace(TRACE3) << __FUNCTION__ << '(' << msgid << ", " << session_id << ')';
    std::string request("DECLARE \n");
    if(!inTestMode())
        request += "   PRAGMA AUTONOMOUS_TRANSACTION; \n";

    request +=
    "   TYPE edi_guts IS RECORD (session_id edi_help.session_id%TYPE,"
                                     " text edi_help.text%TYPE,"
                                  " address edi_help.address%TYPE,"
                                 " eh_rowid ROWID);\n"
    "   TYPE edi_helps IS TABLE OF edi_guts;\n"
    "   eh_all edi_helps;\n"
    "   eh_one edi_helps := edi_helps();\n"
    "BEGIN \n"
    "   SELECT session_id, text, address, rowid\n" // wanna block entire bulk to get :leftover correctly
    "       BULK COLLECT INTO eh_all FROM EDI_HELP\n"
    "       WHERE INTMSGID = :intmsgid AND INSTANCE = :instanceName\n"
    "       FOR UPDATE ORDER BY TIMEOUT;\n"

    "   FOR i IN 1 .. eh_all.COUNT LOOP\n"
    "       IF eh_all(i).session_id = :session_id THEN eh_one.EXTEND(); eh_one(eh_one.COUNT) := eh_all(i); END IF;\n"
    "   END LOOP;\n"

    "   IF eh_one.COUNT < 1 THEN\n"
    "       RAISE_APPLICATION_ERROR(-20101, 'NO EDI_HELP MATCH FOUND');\n"
    "   END IF;\n"

    "   :leftover  := eh_all.COUNT - 1;\n"
    "   :signalTxt := eh_one(1).TEXT;\n"
    "   :address   := eh_one(1).ADDRESS;\n"
    "   DELETE FROM EDI_HELP WHERE ROWID = eh_one(1).eh_rowid;\n";
    if(!inTestMode())
        request += "   COMMIT; \n";

    request += "END; \n";

    ConfirmInfo ci = {};
    char signalTxt[1001];
    char address[41];

    OciCpp::CursCtl cur = make_curs(request);
    cur
        .throwAll()
        .noThrowError(20101)
        .bind(":session_id", session_id)
        .bind(":intmsgid", msgid.id())
        .bind(":instanceName", instance_name)
        .bindOut(":leftover", ci.leftover)
        .bindOut(":signalTxt", signalTxt)
        .bindOut(":address", address)
        .exec();

    LogTrace(TRACE3) << "err = " << cur.err();
    if (cur.err() == 20101) {
        return boost::none;
    } else {
        ci.signalTxt = signalTxt;
        ci.instanceName = instance_name;
        ci.address = address;
        ci.id = msgid.id();
        return ci;
    }
}

boost::optional<EdiHelp> EdiHelpDbOraCallbacks::select_one(const InternalMsgId& msgid, int session_id) const
{
    EdiHelp result;
    EdiHelpSelector selector(result);
    selector.setCondition("WHERE INTMSGID = :intmsgid "
                          "AND SESSION_ID = :session_id ");
    auto cur = selector.make_cursor(STDLOG);
    cur.bind(":intmsgid", msgid.id())
       .bind(":session_id", session_id)
       .EXfet();

    if (cur.err() == NO_DATA_FOUND) {
        LogTrace(TRACE1) << "EdiHelp not found for msgid: " << msgid.asString() << ", session_id: " << session_id;
        return boost::optional<EdiHelp>();
    }

    return result;
}

void EdiHelpDbOraCallbacks::create_db(const EdiHelp &eh, bool clear_other_intmsgid, bool autonomous) const
{
    LogTrace(TRACE5) << __func__ << "(clear_other_intmsgid="<<clear_other_intmsgid<<", autonomous="<<autonomous<<") "
                     << InternalMsgId(eh.id).asString();
    std::string request = "DECLARE \n";

    if(autonomous && !inTestMode())
        request += "   PRAGMA AUTONOMOUS_TRANSACTION; \n";
    request +=  "BEGIN \n"
                "    :delrows := 0; \n"
                "    IF :cleanup_old > 0 THEN \n"
                "        DELETE FROM EDI_HELP WHERE PULT = :pult AND INSTANCE = :instance AND INTMSGID != :id; \n"
                "        :delrows := SQL%ROWCOUNT; \n"
                "    END IF; \n"
                "    INSERT INTO EDI_HELP "
                "        (PULT, INTMSGID, INSTANCE, ADDRESS, TEXT, DATE1, SESSION_ID, TIMEOUT) "
                "        VALUES "
                "        (:pult, :id, :instance, :addr, :txt, :local_time, :sid, :timeout); \n";\
    if(autonomous && !inTestMode())
        request += "    COMMIT; \n";
    request += "END; \n";

    boost::posix_time::ptime local_time = boost::posix_time::second_clock::local_time();
    int delrows = 0;
    auto id = eh.id; // for full bind tmp var
    OciCpp::CursCtl cur = make_curs(request);
    cur
        .bindFull(":id", id.data(), 12, 0, 0, SQLT_BIN)
        .bind(":instance",   eh.instance)
        .bind(":pult",       eh.pult)
        .bind(":local_time", local_time)
        .bind(":timeout",    eh.timeout)
        .bind(":addr",       eh.address)
        .bind(":txt",        eh.txt)
        .bind(":sid",        eh.session_id)
        .bind(":cleanup_old", clear_other_intmsgid > 0 ? 1 : 0)
        .bindOut(":delrows", delrows)
        .exec();

    if (delrows)
        LogWarning(STDLOG) << delrows << " rows deleted for pult " << eh.pult
                           << " msgid: " << InternalMsgId(eh.id);
}

void EdiHelpDbOraCallbacks::clear_old_records() const
{
    LogTrace(TRACE3) << __FUNCTION__;
    const Dates::ptime amin_ago = Dates::second_clock::local_time() - Dates::seconds(60);
    make_curs("delete from edi_help where timeout < :min_ago")
            .bind(":min_ago", OciCpp::to_oracle_datetime(amin_ago))
            .exec();

}

std::list<EdiHelp> EdiHelpDbOraCallbacks::select_all(const char *pult) const
{
    EdiHelp ediHelp;
    EdiHelpSelector selector(ediHelp);
    selector.setCondition("WHERE PULT = :pult ");
    OciCpp::CursCtl cur = make_curs(selector.query().c_str());
    cur
        .bind(":pult", pult);
    selector.def(cur);
    cur.exec();

    std::list<EdiHelp> result;

    while (!cur.fen()) {
        if (ediHelp.binlen != 12) {
            ProgTrace(TRACE1, "binlen = %d", ediHelp.binlen);
            throw comtech::Exception("wrong len");
        }
        result.push_back(ediHelp);
    }

    return result;
}

std::list<EdiHelp> EdiHelpDbOraCallbacks::select_all(const InternalMsgId& msgid) const
{
    EdiHelp ediHelp;
    EdiHelpSelector selector(ediHelp);
    selector.setCondition("WHERE INTMSGID = :intmsgid ");
    auto cur = selector.make_cursor(STDLOG);
    cur.bind(":intmsgid", msgid.id()).exec();

    std::list<EdiHelp> result;

    while (!cur.fen()) {
        if (ediHelp.binlen != 12) {
            ProgTrace(TRACE1, "binlen = %d", ediHelp.binlen);
            throw comtech::Exception("wrong len");
        }
        result.push_back(ediHelp);
    }
    return result;
}

} // namespace ServerFramework
