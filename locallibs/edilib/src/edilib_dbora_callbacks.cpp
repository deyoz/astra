#include <tcl.h>
#include <serverlib/cursctl.h>
#include <serverlib/dates_oci.h>

#include <serverlib/EdiHelpManager.h>
#include <serverlib/int_parameters_oci.h>
#include <serverlib/dates_oci.h>
#include <serverlib/internal_msgid.h>
#include "edilib_dbora_callbacks.h"
#include "edilib/edi_func_cpp.h"
#include "edilib/edi_tables.h"
#include "edilib/edi_session_rc.h"
#include "edilib/edi_sql_insert.h"
#include "edi_sess_except.h"
#include <serverlib/oci_selector_char.h>
#include <boost/optional/optional_io.hpp>

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace edilib
{

struct ESSelector
{
    EdiSession::ReadResult res;
    EdiSession Sess;
    char status[2];
    unsigned system_id;
    std::string key;
    std::string system_type;
    std::string session_type;
    std::string MsgIdString;
    std::string IntMsgIdString;

    edilib::EdiSessionId_t Ida; /*edisession ida*/
    std::string other_carf;
    std::string other_ref;
    int  other_ref_num;
    std::string our_ref;
    int  our_ref_num;
    std::string our_carf;
    std::string pred_p;
    std::string Pult;
    int DbFlags;
    Dates::DateTime_t Timeout;

    static const char *select_text;

    void def(OciCpp::CursCtl &cur);
    int exec(OciCpp::CursCtl &cur, int times_to_wait);

    void get(OciCpp::CursCtl &cur, std::list<EdiSession> &ledisession,
             int times_to_wait  = EdilibDbCallbacks::max_wait_session_times);
    int wait_session_on_cur(OciCpp::CursCtl &cur, int mtrys, unsigned int usec);
    EdiSession::ReadResult get(OciCpp::CursCtl &cur, bool exec_,
                               int times_to_wait  = EdilibDbCallbacks::max_wait_session_times);
};

const char *ESSelector::select_text =
            "SELECT IDA, SYSTEM_ID, SYSTEM_TYPE, SESSION_TYPE, ACCESS_KEY, "
            "OURREF, OURREFNUM, "
            "STATUS, OTHERREF, OTHERREFNUM, OTHERCARF, "
            "PRED_P, INTMSGID, PULT, OURCARF, MSG_ID, FLAGS, TIMEOUT "
            "FROM EDISESSION WHERE ";

inline void sleep_usec(unsigned usec)
{
    usec/=1000;
    if(!usec)
        usec=1;
    Tcl_Sleep(usec);
}

int ESSelector::wait_session_on_cur(OciCpp::CursCtl &cur, int mtrys, unsigned int usec)
{
    int err=CERR_BUSY;
    int num_try=0;

    while(err==CERR_BUSY && num_try<mtrys){
        num_try++;
        ProgTrace(TRACE1,"Num try=%d while waiting for edisession",num_try);

        sleep_usec(usec);

        cur.exec();
        err = cur.err();
        if(!err) {
            break;
        }

    }
    return err;
}

int ESSelector::exec(OciCpp::CursCtl &cur, int times_to_wait)
{
    cur.exec();
    int err = cur.err();
    if(err == CERR_BUSY)
        err = ESSelector::wait_session_on_cur(cur, times_to_wait, EdilibDbCallbacks::usec_sleep_4_sess);
    return err;
}

void ESSelector::def(OciCpp::CursCtl &cur)
{
    cur.
        autoNull().
        noThrowError(CERR_BUSY).
        def(Ida).
        defNull(system_id, unsigned(0)).
        defNull(system_type, "").
        defNull(session_type, "").
        defNull(key, "").
        def(our_ref).
        def(our_ref_num).
        def(status).
        def(other_ref).
        defNull(other_ref_num, 0).
        def(other_carf).
        def(pred_p).
        def(IntMsgIdString).
        def(Pult).
        def(our_carf).
        defNull(MsgIdString, "").
        defNull(DbFlags, 0).
        defNull(Timeout, Dates::DateTime_t(Dates::not_a_date_time));
}

void ESSelector::get(OciCpp::CursCtl &cur, std::list<EdiSession> &ledisession, int times_to_wait)
{
    int err = exec(cur, times_to_wait);
    if(err)
        return;

    while(1) {
        EdiSession::ReadResult res = get(cur, false/*exec*/, times_to_wait);
        if(res.status == EdiSession::NoDataFound)
            break;
        if(res.status == EdiSession::ReadOK)
            ledisession.push_back(res.ediSession.get());
    }
}

EdiSession::ReadResult ESSelector::get(OciCpp::CursCtl &cur, bool exec_, int times_to_wait)
{
    int err = 0;
    if(exec_)
        err = exec(cur, times_to_wait);
    if(!err)
        err = cur.fen();

    if(err) {
        if(err == CERR_NODAT)
        {
            res.status = EdiSession::NoDataFound;
        }
        else if(err==CERR_BUSY)
        {
            res.status = EdiSession::Locked;
        }
    }
    else
    {
        LogTrace(TRACE5) << __func__ << " got EdiSession with " << IntMsgIdString;
        res.status = EdiSession::ReadOK;

        res.ediSession = EdiSession();
        res.ediSession->setStatus(static_cast<edi_act_stat>(status[0]));
        res.ediSession->addFlag(edi_sess_in_base);
        if(system_id && !session_type.empty()) {
            LogTrace(TRACE3) << "SystemId: " << system_id
                             << " key: " << key
                             << " systemType: " << system_type
                             << " sessionType: " << session_type;
            res.ediSession->setSearchKey(EdiSessionSearchKey(system_id, key, system_type, session_type));
        }
        if(not MsgIdString.empty())
            res.ediSession->setMsgId(tlgnum_t(MsgIdString));
        res.ediSession->intmsgid(ServerFramework::InternalMsgId::fromString(IntMsgIdString));
        res.ediSession->setIda(Ida);
        res.ediSession->setOtherCarf(other_carf);
        res.ediSession->setOtherRef(other_ref);
        res.ediSession->setOtherRefNum(other_ref_num);
        res.ediSession->setOurRef(our_ref);
        res.ediSession->setOurRefNum(our_ref_num);
        res.ediSession->setOurCarf(our_carf);
        res.ediSession->setPredp(pred_p);
        res.ediSession->setPult(Pult);
        res.ediSession->addDbFlag(DbFlags);
        res.ediSession->setTimeout(Timeout);
    }
    return res;
}

edilib::EdiSessionId_t EdilibOraCallbacks::ediSessionNextIda() const
{
    OciCpp::CursCtl cur = make_curs("SELECT EDISESSION_SEQ.NEXTVAL FROM DUAL");

    int ida=0;
    cur.def(ida).EXfet();

    return edilib::EdiSessionId_t(ida);
}

unsigned EdilibOraCallbacks::ediSessionNextEdiId() const
{
    unsigned nextid;
    OciCpp::CursCtl cur = make_curs (
            "SELECT edifactid_seq.nextval FROM DUAL");

    cur.def(nextid).EXfet();
    return nextid;
}


void EdilibOraCallbacks::ediSessionWriteDb(EdiSession &edisess) const
{
    const char *req =
            "INSERT INTO EDISESSION"
            " ( OTHERREF, OTHERREFNUM, OURREF, OURREFNUM,"
            " STATUS, IDA, "
            " SYSTEM_ID, SYSTEM_TYPE, SESSION_TYPE, ACCESS_KEY,"
            " SESSDATECR, OURCARF,"
            " OTHERCARF,"
            " LAST_ACCESS, PRED_P, "
            " INTMSGID, PULT, MSG_ID, FLAGS, TIMEOUT)"

            " VALUES (:othref, :othnum, :ourref, :ournum,"
            " :stat, :ida, "
            " :system_id, :system_type, :session_type, :access_key, "
            " SYSDATE, :ourcarf,"
            " :othcarf,"
            " SYSDATE, :predp, "
            " :intmsgid, :pult, :msg_id, :flg, :timeout)";

    if(!edisess.ida().valid())
        throw EdiSessFatal(STDLOG, "edisession.ida is None in writeDb function");

    OciCpp::CursCtl cur = make_curs(req);
    cur.noThrowError(CERR_U_CONSTRAINT);
    char status1[2]="";
    short nnull=0,null=-1;
    const auto search_key = edisess.searchKey();

    status1[0] = edisess.status();
    cur.
            bind(":othref", edisess.otherRef()).
            bind(":othnum", edisess.otherRefNum()).
            bind(":ourref", edisess.ourRef()).
            bind(":ournum", edisess.ourRefNum()).
            bind(":stat", status1).
            bind(":ida",  edisess.ida()).
            bind(":system_id", search_key ? search_key->systemId : 0).
            bind(":system_type", search_key ? search_key->systemType : "").
            bind(":session_type", search_key ? search_key->sessionType : "").
            bind(":access_key", search_key ? search_key->key : "").
            bind(":ourcarf", edisess.ourCarf()).
            bind(":othcarf", edisess.otherCarf()).
            bind(":predp", edisess.predp()).
            bind(":intmsgid",  edisess.intmsgid().asString()).
            bind(":pult",  edisess.pult()).
            bind(":msg_id", edisess.msgId() ? edisess.msgId()->num.get() : "", edisess.msgId() ? &nnull : &null).
            bind(":flg", edisess.dbFlags()).
            bind(":timeout", edisess.timeout()).
            exec();

    if(cur.err())
    {
        edisess.EdiSessNotUpdate();
        if(cur.err() == CERR_U_CONSTRAINT)
        {
            std::ostringstream str;
            str << "Edifact session whith OTHERREF=|" << edisess.otherRef()
                << "| OTHERCARF=|" << edisess.otherCarf()
                << "| OURREF=|" << edisess.ourRef() << " already exists";
            throw EdiSessDup(STDLOG, str.str().c_str());
        }
    }

    std::stringstream  search_key_str;
    if(search_key)
        search_key_str << search_key.get();

    LogTrace(TRACE1) <<
            "Create new edifact session.\n"
            "OTHERREF=|" <<  edisess.otherRef() << "| CARF=|" <<
            edisess.ourCarf() << "/" <<
            edisess.otherCarf() << "| OURREF=|" << edisess.ourRef() << "| MSG_ID=" << edisess.msgId() << ".\n"
            "Ida = " << edisess.ida() << " predp= |" << edisess.predp() << "|"
            " SearchKey = <" << search_key_str.str() << ">"
            " status = |" << status1 << "|"
            " timeout = " << edisess.timeout() << "\n"
            "For pult |" << edisess.pult() << "|" << " DbFlags: " << std::hex << edisess.dbFlags();

    edisess.addFlag(edi_sess_in_base);
}

static EdiSession::ReadResult readByCarf(const std::string &ourcarf,
                                         const std::string &othcarf,
                                         bool update, int times_to_wait)
{
    ProgTrace(TRACE5, "%s", __func__);
    std::string req (ESSelector::select_text);

    if(ourcarf.empty() && othcarf.empty())
    {
        throw EdiSessFatal(STDLOG, "Invalid parameters.");
    }

    if(!ourcarf.empty())
        req += "OURCARF = :OURC ";
    if(!ourcarf.empty() && !othcarf.empty())
        req += " AND ";
    if(!othcarf.empty())
        req += "OTHERCARF = :OTHERC ";
    if(update) {
        req += " FOR UPDATE NOWAIT";
    }

    OciCpp::CursCtl cur = make_curs(req.c_str());

    cur.stb();
    if(!ourcarf.empty())
        cur.bind(":OURC", ourcarf);
    if(!othcarf.empty())
        cur.bind(":OTHERC", othcarf);

    ESSelector selector;
    selector.def(cur);
    return selector.get(cur, true/*exec*/, times_to_wait);
}

EdiSession::ReadResult EdilibOraCallbacks::ediSessionReadByCarf(const std::string &ourcarf,
                                                                const std::string &othcarf, bool update,
                                                                int times_to_wait) const
{
    return readByCarf(ourcarf, othcarf, update, times_to_wait);
}

EdiSession::ReadResult EdilibOraCallbacks::ediSessionReadByIda(edilib::EdiSessionId_t sess_ida, bool update,
                                                               int times_to_wait) const
{
    std::string req (ESSelector::select_text);

    req += "IDA = :ID";
    if(update) {
        req += " FOR UPDATE NOWAIT";
    }

    OciCpp::CursCtl cur = make_curs(req.c_str());

    cur.stb().bind(":ID", sess_ida);

    ESSelector selector;

    selector.def(cur);
    return selector.get(cur, true/*exec*/, times_to_wait);
}

EdiSession::ReadResult EdilibOraCallbacks::ediSessionReadByKey(const EdiSessionSearchKey &key, bool update) const
{
    std::string req (ESSelector::select_text);

    req += "SYSTEM_ID = :SYSTEM_ID"
            " AND SYSTEM_TYPE = :SYSTEM_TYPE"
            " AND SESSION_TYPE = :SESSION_TYPE"
            " AND ACCESS_KEY = :ACCESS_KEY";

    if(update)
        req += " FOR UPDATE NOWAIT";

    OciCpp::CursCtl cur = make_curs(req.c_str());
    cur.
            stb().
            bind(":SYSTEM_ID", key.systemId).
            bind(":SYSTEM_TYPE", key.systemType).
            bind(":SESSION_TYPE", key.sessionType).
            bind(":ACCESS_KEY", key.key);

    LogTrace(TRACE3) << "Search edisession by " << key;

    ESSelector selector;
    selector.def(cur);
    return selector.get(cur, true/*exec*/);
}

void EdilibOraCallbacks::ediSessionReadByKey(std::list<EdiSession> &ledisession,
                                             const std::string &key, const std::string &sessionType,
                                             bool update) const
{
    std::string req (ESSelector::select_text);

    req += "SESSION_TYPE = :SESSION_TYPE"
            " AND ACCESS_KEY = :ACCESS_KEY";

    if(update)
        req += " FOR UPDATE NOWAIT";

    OciCpp::CursCtl cur = make_curs(req.c_str());
    cur.
            stb().
            bind(":SESSION_TYPE", sessionType).
            bind(":ACCESS_KEY", key);

    LogTrace(TRACE3) << "Search edisession by " << key << "/" << sessionType;

    ESSelector selector;
    selector.def(cur);
    return selector.get(cur, ledisession);
}

void EdilibOraCallbacks::ediSessionUpdateDb(EdiSession &edisession) const
{
    std::string req =
        "UPDATE EDISESSION "
                "SET OURREFNUM=:ournum, "
                "OTHERREFNUM=:othnum, "
                "OTHERREF=:othref, "
                "OURCARF=:ourcarf, "
                "OTHERCARF=:othcarf, "
                "STATUS=:stat, "
                "LAST_ACCESS=sysdate, "
                "TIMEOUT=:timeout, "
                "MSG_ID = :msg_id, "
                "FLAGS = :flg, "
                "INTMSGID = :intmsgid, "
                "PULT=:pult";

    if(!edisession.ourRef().empty())
        req += ", OURREF=:ourref";


    req += " WHERE IDA = :ID";

    char status1[2]=" ";
    *status1 = edisession.status();
    auto IntMsgIdStr = edisession.intmsgid().asString();
    auto msgid = edisession.msgId() ? edisession.msgId()->num.get() : "";
    auto ourref = edisession.ourRef();

    LogTrace(TRACE1) << "Update edisession set <"
        << edisession.ourRefNum() << "><"
        << edisession.otherRefNum() << "><"
        << edisession.otherRef().c_str() << "><"
        << edisession.ourCarf().c_str() << "><"
        << edisession.otherCarf().c_str() << "><"
        << ourref << "><"
        << edisession.intmsgid().asString() << "><"
        << status1 << "><msg_id="
        << msgid << "><flags="
        << std::hex << std::showbase << edisession.dbFlags() << std::noshowbase
        << "><IntMsgIdStr=" << IntMsgIdStr
        << "><pult=" << edisession.pult()
        << "><timeout = " << edisession.timeout()
        << "> for ida="
        << edisession.ida().get();

    OciCpp::CursCtl cur = make_curs(req);

    cur.noThrowError(CERR_U_CONSTRAINT);
    if(!ourref.empty())
        cur.bind(":ourref", ourref);

    cur.
            bind(":ournum", edisession.ourRefNum()).
            bind(":othnum", edisession.otherRefNum()).
            bind(":othref", edisession.otherRef()).
            bind(":ourcarf",edisession.ourCarf()).
            bind(":othcarf",edisession.otherCarf()).
            bind(":stat",   status1).
            bind(":timeout", edisession.timeout()).
            bind(":intmsgid",IntMsgIdStr).
            bind(":pult",   edisession.pult()).
            bind(":flg",    edisession.dbFlags()).
            bind(":ID",     edisession.ida()).
            bind(":msg_id", msgid).
            exec();

    if(cur.err() == CERR_U_CONSTRAINT){
        edisession.EdiSessNotUpdate();
        std::ostringstream str;

        str << "Unique constraint on session: other_carf<" <<
                edisession.otherCarf() << ">-our_carf<" <<
                edisession.ourCarf() << ">";
        throw EdiSessDup(STDLOG,str.str().c_str());
    }

    edisession.addFlag(edi_sess_in_base);
}

void EdilibOraCallbacks::ediSessionDeleteDb(edilib::EdiSessionId_t Id) const
{
    OciCpp::CursCtl cur = make_curs("delete from edisession where ida=:ID");

    cur.
            bind(":ID", Id).
            exec();

    if(cur.rowcount() != 1)
        LogError(STDLOG) << "Delete from edisession by ida = " << Id << ", No rows processed!";
}

bool EdilibOraCallbacks::ediSessionIsExists(edilib::EdiSessionId_t sess_ida) const
{
    OciCpp::CursCtl cur = make_curs("SELECT 1 FROM EDISESSION WHERE IDA = :ID");
    cur.bind(":ID", sess_ida).EXfet();
    if(cur.err() == CERR_NODAT)
        return false;
    else
        return true;
}

struct EdiSessToDefFields
{
    static const char *select_text;
    EdiSessionId_t SessId;
    std::string fCode;
    std::string mName;
    Dates::DateTime_t tout;

    void dodef(OciCpp::CursCtl &cur)
    {
        cur.
            def(SessId).
            def(mName).
            defNull(fCode, "").
            def(tout);
    }
    EdiSessionTimeOut make()
    {
        return EdiSessionTimeOut(GetEdiMsgTypeByName(mName.c_str()),
                                 fCode, SessId,
                                 expiredSecs(tout));
    }
};

const char *EdiSessToDefFields::select_text =
            "SELECT SESS_IDA, MSG_NAME, FUNC_CODE, TIME_OUT from "
            "EDISESSION_TIMEOUTS";

EdiSessionTimeOut EdilibOraCallbacks::ediSessionToReadById(EdiSessionId_t id) const
{
    OciCpp::CursCtl cur =
            make_curs ((std::string(EdiSessToDefFields::select_text) + " where SESS_IDA = :SID").c_str());

    EdiSessToDefFields deff;
    deff.dodef(cur);
    cur.
            throwAll().
            bind(":SID", id).
            EXfet();
    return deff.make();
}

void EdilibOraCallbacks::ediSessionToWriteDb(const EdiSessionTimeOut &edisess_to) const
{
    OciCpp::CursCtl cur =
            make_curs("insert into EDISESSION_TIMEOUTS "
                      "(SESS_IDA, MSG_NAME, FUNC_CODE, TIME_OUT, INSTANCE) "
                      "values "
                      "(:SID, :NAME, :FC, :TOUT, :INSTANCE)");
    cur.
            bind(":SID", edisess_to.ediSessionId()).
            bind(":NAME", edisess_to.msgTypeStr()).
            bind(":FC", edisess_to.funcCode()).
            bind(":INSTANCE", ServerFramework::EdiHelpManager::instanceName()).
            bind(":TOUT",  getTimeOut(edisess_to.timeout())).
            exec();
}

bool EdilibOraCallbacks::ediSessionToIsExists(EdiSessionId_t Id) const
{
    OciCpp::CursCtl cur = make_curs ("select 1 from EDISESSION_TIMEOUTS where SESS_IDA = :SID");
    cur.bind(":SID", Id).EXfet();
    return cur.err() != NO_DATA_FOUND;
}

void EdilibOraCallbacks::ediSessionToReadExpired(std::list<EdiSessionTimeOut> & lExpired) const
{
    using namespace boost::posix_time;
    OciCpp::CursCtl cur =
            make_curs ((std::string(EdiSessToDefFields::select_text) +
                        " where time_out <= :date1 and instance = :instance").c_str());

    cur.
            stb().
            bind(":date1",    Dates::currentDateTime()).
            bind(":instance", ServerFramework::EdiHelpManager::instanceName());
    EdiSessToDefFields deff;
    deff.dodef(cur);
    cur.exec();

    while(!cur.fen())
    {
        lExpired.push_back(deff.make());
        LogTrace(TRACE1) << "Got expired edisession: " << lExpired.back();
    }

    LogTrace(TRACE1) << "Got " << lExpired.size() << " expired sessions";
}

void EdilibOraCallbacks::ediSessionToDeleteDb(EdiSessionId_t sessid) const
{
    OciCpp::CursCtl cur = make_curs("delete from EDISESSION_TIMEOUTS where sess_ida=:id");

    cur.
            bind(":id", sessid).
            exec();

    if(!cur.rowcount())
    {
        LogWarning(STDLOG) << cur.rowcount() << " records were deleted from EDISESSION_TIMEOUTS by "
            "edisession.id = " << sessid;
    }
}

void EdilibOraCallbacks::readMesTableData(std::vector<_MESSAGE_TABLE_STRUCT_> &vMesTable) const
{
    const char *Text=
            "SELECT RTRIM(MESSAGE),RTRIM(TEXT) "
            "FROM EDI_MESSAGES";

    MESSAGE_TABLE_STRUCT MesTableTmp = {};
    OciCpp::CursCtl cur = make_curs(Text);
    cur.
        def(MesTableTmp.Message).
        def(MesTableTmp.Text).
        exec();

    while(!cur.fen()) {
        vMesTable.push_back(MesTableTmp);
    }
}

void EdilibOraCallbacks::readMesStrTableData(std::vector<_MES_STRUCT_TABLE_STRUCT_> &vMesStrTable) const
{
    const char *Text=
            "SELECT RTRIM(MESSAGE), POS, RTRIM(TAG), "
            "MAXPOS, S, R, GRP_NUM "
            "FROM EDI_STR_MESSAGE ORDER BY MESSAGE, POS";

    OciCpp::CursCtl cur = make_curs(Text);

    MES_STRUCT_TABLE_STRUCT MesStrTable = {};
    cur.
            def(MesStrTable.Message).
            def(MesStrTable.Pos).
            defNull(MesStrTable.Tag, "").
            defNull(MesStrTable.MaxPos, 0).
            def(MesStrTable.S).
            def(MesStrTable.R).
            defNull(MesStrTable.GrpNum, 0).
            exec();

    while(!cur.fen()) {
        vMesStrTable.push_back(MesStrTable);
    }
}

void EdilibOraCallbacks::readSegStrTableData(std::vector<_SEG_STRUCT_TABLE_STRUCT_> &vSegStrTable) const
{
    const char *Text =
            "SELECT RTRIM(TAG), POS, S, R, "
            "COMPOSITE, DATAELEMENT "
            "FROM EDI_STR_SEGMENT ORDER BY TAG, POS";

    SEG_STRUCT_TABLE_STRUCT SegStrTmp = {};
    OciCpp::CursCtl cur = make_curs(Text);
    cur.
        def(SegStrTmp.Tag).
        def(SegStrTmp.Pos).
        def(SegStrTmp.S).
        def(SegStrTmp.R).
        defNull(SegStrTmp.Composite, "").
        defNull(SegStrTmp.DataElem, 0).
        exec();

    while(!cur.fen()) {
        vSegStrTable.push_back(SegStrTmp);
    }
}

void EdilibOraCallbacks::readCompStrTableData(std::vector<_COMP_STRUCT_TABLE_STRUCT_> &vCompStrTable) const
{
    const char *Text=
            "SELECT COMPOSITE, POS, S, R, "
            "DATAELEMENT "
            "FROM EDI_STR_COMPOSITE ORDER BY COMPOSITE, POS";

    OciCpp::CursCtl cur = make_curs(Text);

    COMP_STRUCT_TABLE_STRUCT CompStr = {};
    cur.
        def(CompStr.Composite).
        def(CompStr.Pos).
        def(CompStr.S).
        def(CompStr.R).
        def(CompStr.DataElem).
        exec();

    while(!cur.fen()) {
        vCompStrTable.push_back(CompStr);
    }
}

void EdilibOraCallbacks::readDataElemTableData(std::vector<_DATA_ELEM_TABLE_STRUCT_> &vDataTable) const
{
    const char *Text=
            "SELECT DATAELEMENT,RTRIM(TEXT), "
            "FORMAT, MINFIELD, MAXFIELD "
            "FROM EDI_DATA_ELEM";

    DATA_ELEM_TABLE_STRUCT DataElem = {};
    OciCpp::CursCtl cur = make_curs(Text);
    cur.
        def(DataElem.DataElem).
        def(DataElem.Text).
        def(DataElem.Format).
        def(DataElem.MinField).
        def(DataElem.MaxField).
        exec();

    while(!cur.fen()) {
        vDataTable.push_back(DataElem);
    }
}

void EdilibOraCallbacks::readSegTableData(std::vector<_SEGMENT_TABLE_STRUCT_> &vSegTable) const
{
    const char *Text=
            "SELECT RTRIM(TAG),RTRIM(TEXT) "
            "FROM EDI_SEGMENT";

    SEGMENT_TABLE_STRUCT Seg = {};
    OciCpp::CursCtl cur = make_curs(Text);
    cur.
        def(Seg.Tag ).
        def(Seg.Text).
        exec();

    while(!cur.fen()) {
        vSegTable.push_back(Seg);
    }
}

void EdilibOraCallbacks::readCompTableData(std::vector<_COMPOSITE_TABLE_STRUCT_> &vCompTable) const
{
    const char *Text=
            "SELECT RTRIM(COMPOSITE),RTRIM(TEXT) "
            "FROM EDI_COMPOSITE";

    OciCpp::CursCtl cur = make_curs(Text);

    COMPOSITE_TABLE_STRUCT Comp = {};
    cur.
        def(Comp.Composite).
        def(Comp.Text).
        exec();

    while(!cur.fen()) {
        vCompTable.push_back(Comp);
    }
}


void EdilibOraCallbacks::ediSessionRcWriteDb(const EdiSessionRc &edisessRc) const
{
    LogTrace( TRACE3 ) << "write resource_control to DB: " << edisessRc;
    OciCpp::CursCtl cur =
        make_curs( "insert into EDISESSION_RC (SESS_IDA, TYPE, TIME_OUT) "
                   "values (:SID, :TYPE, :TOUT)" );
    cur.
        bind( ":SID", edisessRc.sessionId() ).
        bind( ":TYPE", edisessRc.type() ).
        bind( ":TOUT", getTimeOut( edisessRc.timeOut() ) ).
        exec();
}

void EdilibOraCallbacks::clearMesTableData() const
{
    const char *sql =
            "begin\n"
            "delete from edi_messages;\n"
            "delete from edi_str_message;\n"
            "delete from edi_segment;\n"
            "delete from edi_str_segment;\n"
            "delete from edi_composite;\n"
            "delete from edi_str_composite;\n"
            "delete from edi_data_elem;\n"
            "end;";

    make_curs(sql).throwAll().exec();
}

void EdilibOraCallbacks::insertMesTableData(_COMMAND_STRUCT_ *pCommStr) const
{
    const char *Text =
            "INSERT INTO EDI_MESSAGES(MESSAGE, TEXT) "
            "VALUES (:M,:T)";

    OciCpp::CursCtl cur = make_curs(Text);
    cur.noThrowError(SQL_DUBLICATE);
    cur.bind(":M", pCommStr->Command[0])
       .bind(":T", pCommStr->Text)
       .exec();

    if(cur.err() == SQL_DUBLICATE){
        EdiTrace( TRACE3,"Dublicate %s in EDI_MESSAGES", pCommStr->Command[0]);
    }
}

void EdilibOraCallbacks::insertMesStrTableData(_COMMAND_STRUCT_ *pCommStr) const
{
    const char *Text =
            "INSERT INTO EDI_STR_MESSAGE"
            "(MESSAGE, POS, TAG, MAXPOS, S, R, GRP_NUM) "
            "VALUES (:M, :P, :T, :MP, :S, :R, :GN)";

    OciCpp::CursCtl cur = make_curs(Text);
    cur.
        noThrowError(SQL_DUBLICATE).
        bind(":M" , pCommStr->Command[0]).
        bind(":P" , pCommStr->Command[1]).
        bind(":T" , pCommStr->Command[2]).
        bind(":MP", pCommStr->Command[3]).
        bind(":S" , pCommStr->Command[4]).
        bind(":R" , pCommStr->Command[5]).
        bind(":GN", pCommStr->Command[6]).
        exec();

    if(cur.err() == SQL_DUBLICATE) {
        EdiTrace( TRACE3,"Dublicate %s -> %s in EDI_STR_MESSAGE",
                  pCommStr->Command[0], pCommStr->Command[2]);
    }
}

void EdilibOraCallbacks::insertSegTableData(_COMMAND_STRUCT_ *pCommStr) const
{
    const char *Text =
            "INSERT INTO EDI_SEGMENT (TAG, TEXT) "
            "VALUES (:T, :Txt)";

    OciCpp::CursCtl cur = make_curs(Text);
    cur.
        noThrowError(SQL_DUBLICATE).
        bind(":T"  , pCommStr->Command[0]).
        bind(":Txt", pCommStr->Text      ).
        exec();

    if(cur.err() == SQL_DUBLICATE) {
        EdiTrace( TRACE3,"Dublicate %s in EDI_SEGMENT", pCommStr->Command[0]);
    }
}

void EdilibOraCallbacks::insertSegStrTableData(_COMMAND_STRUCT_ *pCommStr) const
{
    const char *Text =
            "INSERT INTO EDI_STR_SEGMENT"
            "(TAG, POS, S, R, COMPOSITE, DATAELEMENT) "
            "VALUES (:TAG, TO_NUMBER(:POS), :S, TO_NUMBER(:R), "
            ":COMP, TO_NUMBER(:DE))";

    OciCpp::CursCtl cur = make_curs(Text);

    cur.noThrowError(SQL_DUBLICATE).
        bind(":TAG"  , pCommStr->Command[0]).
        bind(":POS"  , pCommStr->Command[1]).
        bind(":S"    , pCommStr->Command[4]).
        bind(":R"    , pCommStr->Command[5]).
        bind(":COMP" , pCommStr->Command[2]).
        bind(":DE"   , pCommStr->Command[3]).
        exec();

    if(cur.err() == SQL_DUBLICATE) {
        EdiTrace( TRACE3,"Dublicate %s -> %s in EDI_STR_SEGMENT",
                  pCommStr->Command[0], pCommStr->Command[2]);
    }
}

void EdilibOraCallbacks::insertCompTableData(_COMMAND_STRUCT_ *pCommStr) const
{
    const char *Text=
            "INSERT INTO EDI_COMPOSITE"
            "(COMPOSITE, TEXT) VALUES (:COMP, :TXT)";

    OciCpp::CursCtl cur = make_curs(Text);
    cur.
        noThrowError(SQL_DUBLICATE).
        bind(":COMP"  , pCommStr->Command[0]).
        bind(":TXT"   , pCommStr->Text      ).
        exec();

    if(cur.err() == SQL_DUBLICATE) {
        EdiTrace( TRACE3,"Dublicate %s in EDI_COMPOSITE", pCommStr->Command[0]);
    }
}

void EdilibOraCallbacks::insertCompStrTableData(_COMMAND_STRUCT_ *pCommStr) const
{
    const char *Text=
            "INSERT INTO EDI_STR_COMPOSITE"
            "(COMPOSITE, POS, S, R, DATAELEMENT) "
            "VALUES (:COMP, TO_NUMBER( :POS ), :S, "
            "TO_NUMBER( :R ), TO_NUMBER( :DE ) )";

    OciCpp::CursCtl cur = make_curs(Text);

    cur.noThrowError(SQL_DUBLICATE).
        bind(":COMP" , pCommStr->Command[0]).
        bind(":POS"  , pCommStr->Command[1]).
        bind(":S"    , pCommStr->Command[3]).
        bind(":R"    , pCommStr->Command[4]).
        bind(":DE"   , pCommStr->Command[2]).
        exec();
    if(cur.err() == SQL_DUBLICATE) {
        EdiTrace( TRACE3,"Dublicate %s -> %s in EDI_STR_COMPOSITE",
                  pCommStr->Command[0], pCommStr->Command[2]);
    }
}

void EdilibOraCallbacks::insertDataElemTableData(_COMMAND_STRUCT_ *pCommStr) const
{
    const char *Text=
            "INSERT INTO EDI_DATA_ELEM"
            "(DATAELEMENT, TEXT, FORMAT, MINFIELD, MAXFIELD) "
            "VALUES (TO_NUMBER(:DE), :TXT, :F, TO_NUMBER(:MINF), TO_NUMBER(:MAXF))";

    OciCpp::CursCtl cur = make_curs(Text);

    cur.
        noThrowError(SQL_DUBLICATE).
        bind(":DE"  , pCommStr->Command[0]).
        bind(":TXT" , pCommStr->Text      ).
        bind(":F"   , pCommStr->Command[1]).
        bind(":MINF", pCommStr->Command[2]).
        bind(":MAXF", pCommStr->Command[3]).
        exec();

    if(cur.err() == SQL_DUBLICATE) {
        EdiTrace( TRACE3,"Dublicate %s in EDI_DATA_ELEM", pCommStr->Command[0]);
    }
}

struct EdiSessionRcDefFields
{
    static const char* select_text;
    EdiSessionId_t SessId;
    std::string type;
    Dates::DateTime_t tout;
    void doDef( OciCpp::CursCtl &cur )
    {
        cur.
            def( SessId ).
            def( type ).
            def( tout );
    }
    EdiSessionRc make()
    {
        return EdiSessionRc( SessId, type, expiredSecs(tout) );
    }
};

const char* EdiSessionRcDefFields::select_text =
            "select SESS_IDA, TYPE, TIME_OUT from "
            "EDISESSION_RC ";


void EdilibOraCallbacks::ediSessionRcDeleteDb(const EdiSessionId_t& sessId) const
{
    LogTrace( TRACE3 ) << "delete resource_control from DB for sessId " << sessId;
    OciCpp::CursCtl cur = make_curs( "delete from EDISESSION_RC where SESS_IDA=:SID" );
    cur.
        bind( ":SID", sessId ).
        exec();

    if( !cur.rowcount() )
    {
        LogError( STDLOG ) << cur.rowcount() << " records were deleted from EDISESSION_RC by"
                                                " edisession.id = " << sessId;
    }
}

bool EdilibOraCallbacks::ediSessionRcIsExists(const EdiSessionId_t& sessId) const
{
    OciCpp::CursCtl cur = make_curs( "select 1 from EDISESSION_RC where SESS_IDA = :SID" );
    cur.bind( ":SID", sessId ).EXfet();
    return ( cur.err() != NO_DATA_FOUND );
}

EdiSessionRc EdilibOraCallbacks::ediSessionRcReadById(const EdiSessionId_t& sessId) const
{
    OciCpp::CursCtl cur =
            make_curs((std::string( EdiSessionRcDefFields::select_text ) +
                      " where SESS_IDA = :SID" ));

    EdiSessionRcDefFields deff;
    deff.doDef( cur );
    cur.
        throwAll().
        bind(":SID", sessId).
        EXfet();

    return deff.make();
}

void EdilibOraCallbacks::ediSessionRcReadExpired(std::list< EdiSessionRc >& lExpired) const
{
    using namespace boost::posix_time;
    OciCpp::CursCtl cur =
            make_curs((std::string(EdiSessionRcDefFields::select_text) + " where TIME_OUT <= :date1" ));
    cur.
        stb().
        bind(":date1", second_clock::local_time());

    EdiSessionRcDefFields deff;
    deff.doDef( cur );
    cur.exec();

    while( !cur.fen() )
    {
        lExpired.push_back( deff.make() );
        LogTrace( TRACE1 ) << "Got expired edisession rc: " << lExpired.back();
    }

    LogTrace( TRACE1 ) << "Got " << lExpired.size() << " expired resource_controls";
}

void EdilibOraCallbacks::commit() const
{
    OciCpp::mainSession().commit();
}

void EdilibOraCallbacks::rollback() const
{
    OciCpp::mainSession().rollback();
}

} // namespace edilib
