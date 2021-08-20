#include "remote_results.h"
#include "exceptions.h"
#include "PgOraConfig.h"
#include "pg_session.h"

#ifdef ENABLE_ORACLE
#include "astra_dates_oci.h"
#include <serverlib/oci8cursor.h>
#include <serverlib/int_parameters_oci.h>
#endif
#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>

#include <tclmon/internal_msgid.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace edifact;

const char *edifact::RemoteStatusElem::ElemName = "Remote action status";

DESCRIBE_CODE_SET(edifact::RemoteStatusElem)
{
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::CommonError,
                             "CE",
                             "Error in remote host",
                             "Ошибка обработки в удаленном хосте"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::Contrl,
                             "CL",
                             "Communication error (CONTRL)",
                             "Ошибка связи (CONTRL)"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::Timeout,
                             "TO",
                             "Time out",
                             "Time out"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::Success,
                             "SS",
                             "Success",
                             "Успешно"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::RequestSent,
                             "RS",
                             "Request was sent",
                             "Запрос обрабатывается удаленным хостом"));
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace edifact {

using namespace OciCpp;

struct oradefsupport
{
    std::string MsgId;
    std::string Pult;
    std::string Status;
    std::string Remark;
    std::string EdiErrCode;
    Dates::DateTime_t DateCr;
    RemoteResults::RawData_t RawData;
    edilib::EdiSessionId_t::base_type   EdiSession;
    Ticketing::SystemAddrs_t::base_type RemoteSystem;

    RemoteResults make()
    {
        RemoteResults rr;
        rr.DateCr       = DateCr;
        rr.Status       = RemoteStatus(Status);
        rr.MsgId        = MsgId;
        rr.Pult         = Pult;
        rr.EdiSession   = edilib::EdiSessionId_t(EdiSession);
        rr.RemoteSystem = Ticketing::SystemAddrs_t(RemoteSystem);
        rr.Remark       = Remark;
        rr.EdiErrCode   = EdiErrCode;
        rr.TlgSource    = RawData;
        return rr;
    }
};

struct pgdefsupport
{
    std::string MsgId;
    std::string Pult;
    std::string Status;
    std::string Remark;
    std::string EdiErrCode;
    Dates::DateTime_t DateCr;
    std::string RawData;
    short RawDataInd = 0;
    PgCpp::BinaryDefHelper<std::string> RawDataDef{RawData};
    edilib::EdiSessionId_t::base_type   EdiSession;
    Ticketing::SystemAddrs_t::base_type RemoteSystem;

    RemoteResults make()
    {

        RemoteResults rr = {};
        rr.DateCr       = DateCr;
        rr.Status       = RemoteStatus(Status);
        rr.MsgId        = MsgId;
        rr.Pult         = Pult;
        rr.EdiSession   = edilib::EdiSessionId_t(EdiSession);
        rr.RemoteSystem = Ticketing::SystemAddrs_t(RemoteSystem);
        rr.Remark       = Remark;
        rr.EdiErrCode   = EdiErrCode;
        if(!RawDataInd) {
            rr.TlgSource = RemoteResults::RawData_t(RawData.begin(), RawData.end());
        }
        return rr;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////

namespace
{

const std::string Select =
        "select INTMSGID, PULT, STATUS, DATE_CR, EDISESSION_ID, REMOTE_ID, "
        "REMARK, EDIERRCODE, RAWDATA from REMOTE_RESULTS ";

const std::string Insert =
        "insert into REMOTE_RESULTS "
        "(INTMSGID, PULT, STATUS, DATE_CR, EDISESSION_ID, REMOTE_ID) "
        "values "
        "(:INTMSGID, :PULT, :STATUS, :LOCAL_TIME, :EDISESSION_ID, :REMOTE_ID)";

const std::string Update =
        "update REMOTE_RESULTS set "
        "EDIERRCODE = :EDIERRCODE,"
        "STATUS = :STATUS,"
        "REMARK = :REMARK,"
        "RAWDATA = :RAWDATA ";

const std::string Delete =
        "delete from REMOTE_RESULTS ";


#ifdef ENABLE_ORACLE
inline void OciDef(Curs8Ctl &cur, oradefsupport &def)
{
    cur
            .def(def.MsgId)
            .def(def.Pult)
            .def(def.Status)
            .def(def.DateCr)
            .def(def.EdiSession)
            .def(def.RemoteSystem)
            .def(def.Remark)
            .def(def.EdiErrCode)
            .defBlob(def.RawData);
}
#endif //ENABLE_ORACLE

inline void PgDef(PgCpp::CursCtl & cur, pgdefsupport &def)
{
    cur
            .defNull(def.MsgId, "")
            .defNull(def.Pult, "")
            .defNull(def.Status, "")
            .def(def.DateCr)
            .def(def.EdiSession)
            .def(def.RemoteSystem, 0)
            .defNull(def.Remark, "")
            .defNull(def.EdiErrCode, "")
            .def(def.RawDataDef, &def.RawDataInd);
}

} //namespace


/////////////////////////////////////////////////////////////////////////////////////////

RemoteResults::RemoteResults(const std::string &msgId,
                             const std::string &pult,
                             const edilib::EdiSessionId_t &edisess,
                             const Ticketing::SystemAddrs_t &remoteId)
    : MsgId(msgId),
      Pult(pult),
      EdiSession(edisess),
      Status(RemoteStatus::RequestSent),
      RemoteSystem(remoteId)
{}

std::string RemoteResults::tlgSource() const
{
    if(TlgSource.empty()) {
        return std::string();
    }
    return std::string(TlgSource.data(), TlgSource.size());
}

void RemoteResults::setTlgSource(const std::string &tlg)
{
    TlgSource.resize(tlg.length());
    memcpy(&TlgSource[0], tlg.c_str(), tlg.length());
}

void RemoteResults::add(const std::string &msgId,
                        const std::string &pult,
                        const edilib::EdiSessionId_t &edisess,
                        const Ticketing::SystemAddrs_t &remoteId)
{
    RemoteResults tmp(msgId, pult, edisess, remoteId);
    //tmp.removeOld(); неприемлемо
    tmp.writeDb();
}

static void readDbOra(std::list<RemoteResults>& lres, const std::string& msgId)
{
#ifdef ENABLE_ORACLE
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
                 Select + " where INTMSGID=:INTID", &os);

    oradefsupport defs;
    cur
            .bind(":INTID", msgId);
    OciDef(cur, defs);
    cur.exec();

    while(!cur.fen()) {
        lres.push_back(defs.make());
    }

    if(!lres.empty())
    {
        const auto query = Delete + " where INTMSGID=:INTID";
        Curs8Ctl(STDLOG, query, &os)
                .bind(":INTID", msgId)
                .exec();
    }
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

static void readDbPg(std::list<RemoteResults>& lres, const std::string& msgId)
{
    LogTrace(TRACE7) << __func__ << " by " << msgId;
    const auto query = Select + " where INTMSGID=:INTID";
    PgCpp::CursCtl cur = make_pg_curs(PgCpp::getPgManaged(), query);

    pgdefsupport defs;
    PgDef(cur, defs);
    cur
            .bind(":INTID", msgId)
            .exec();

    while(!cur.fen()) {
        lres.push_back(defs.make());
    }

    if(!lres.empty())
    {
        const auto query = Delete + " where INTMSGID=:INTID";
        make_pg_curs(PgCpp::getPgManaged(), query)
                .bind(":INTID", msgId)
                .exec();
    }
}

/*
для Астры неприемлемо завязываться на пульт, только на msgid либо на ид. edifact сессии
из-за того что от имени одного пульта могут одновременно работать много независимых edifact запросов!
*/
void RemoteResults::readDb(/*const std::string &pult,*/ std::list<RemoteResults> &lres)
{
    auto msgid = ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString();

    if(PgOra::supportsPg("REMOTE_RESULTS")) {
        readDbPg(lres, msgid);
    } else {
        readDbOra(lres, msgid);
    }
}
/*
для Астры неприемлемо завязываться на пульт, только на msgid либо на ид. edifact сессии
из-за того что от имени одного пульта могут одновременно работать много независимых edifact запросов!
*/
pRemoteResults RemoteResults::readSingle(/*const std::string &pult*/)
{
    std::list<RemoteResults> lres;
    readDb(lres);
    if(lres.size() != 1)
    {
        LogError(STDLOG) << "Got " << lres.size() << " for " <<
                " MsgId: " << ServerFramework::getQueryRunner().getEdiHelpManager().msgId() <<
                " remote results while one was expected";
        throw EXCEPTIONS::Exception("Failed to read results"); // TODO throw UserException
    }
    else
    {
        return pRemoteResults(new RemoteResults(lres.front()));
    }
}

static pRemoteResults readDbOra(const edilib::EdiSessionId_t& Id)
{
#ifdef ENABLE_ORACLE
    oradefsupport defs;
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
                 Select + " where EDISESSION_ID = :EDISESS_ID", &os);

    OciDef(cur, defs);
    cur
            .bind(":EDISESS_ID", Id.get())
            .EXfet();

    if(cur.rowcount() == 0) {
        return pRemoteResults();
    } else {
        return pRemoteResults(new RemoteResults(defs.make()));
    }
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

static pRemoteResults readDbPg(const edilib::EdiSessionId_t& Id)
{
    LogTrace(TRACE7) << __func__ << " by " << Id;
    const auto query = Select + " where EDISESSION_ID = :EDISESS_ID";
    PgCpp::CursCtl cur = make_pg_curs(PgCpp::getPgManaged(), query);

    pgdefsupport defs;
    PgDef(cur, defs);
    cur
            .bind(":EDISESS_ID", Id.get())
            .EXfet();

    if(cur.rowcount() == 0) {
        return pRemoteResults();
    } else {
        return pRemoteResults(new RemoteResults(defs.make()));
    }
}

pRemoteResults RemoteResults::readDb(const edilib::EdiSessionId_t &Id)
{
    if(PgOra::supportsPg("REMOTE_RESULTS")) {
        return readDbPg(Id);
    } else {
        return readDbOra(Id);
    }
}

static void writeDbOra(const RemoteResults& rr)
{
    LogTrace(TRACE7) << __func__ << std::endl << rr;
#ifdef ENABLE_ORACLE
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG, Insert, &os);
    cur
            .bind(":INTMSGID",      rr.msgId())
            .bind(":PULT",          rr.pult())
            .bind(":STATUS",        rr.status()->code())
            .bind(":LOCAL_TIME",    Dates::second_clock::local_time())
            .bind(":EDISESSION_ID", rr.ediSession().get())
            .bind(":REMOTE_ID",     rr.remoteSystem().get())
            .exec();
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

static void writeDbPg(const RemoteResults& rr)
{
    LogTrace(TRACE7) << __func__ << std::endl << rr;
    PgCpp::CursCtl cur = make_pg_curs(PgCpp::getPgManaged(), Insert);
    cur
            .stb()
            .bind(":INTMSGID",      rr.msgId())
            .bind(":PULT",          rr.pult())
            .bind(":STATUS",        rr.status()->code())
            .bind(":LOCAL_TIME",    Dates::second_clock::local_time())
            .bind(":EDISESSION_ID", rr.ediSession().get())
            .bind(":REMOTE_ID",     rr.remoteSystem().get())
            .exec();
}

void RemoteResults::writeDb()
{
    if(PgOra::supportsPg("REMOTE_RESULTS")) {
        writeDbPg(*this);
    } else {
        writeDbOra(*this);
    }
}

static void updateDbOra(const RemoteResults& rr)
{
    LogTrace(TRACE7) << __func__ << std::endl << rr;
#ifdef ENABLE_ORACLE
    const auto query = Update + " where EDISESSION_ID = :EDISESS_ID";
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG, query, &os);
    cur
            .bind(":EDIERRCODE",  rr.ediErrCode())
            .bind(":STATUS",      rr.status()->code())
            .bind(":REMARK",      rr.remark())
            .bindBlob(":RAWDATA", rr.rawData())
            .bind(":EDISESS_ID",  rr.ediSession().get())
            .exec();
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

static void updateDbPg(const RemoteResults& rr)
{
    LogTrace(TRACE7) << __func__ << std::endl << rr;
    const auto query = Update + " where EDISESSION_ID = :EDISESS_ID";
    PgCpp::CursCtl cur = make_pg_curs(PgCpp::getPgManaged(), query);
    cur
            .stb()
            .bind(":EDIERRCODE", rr.ediErrCode())
            .bind(":STATUS",     rr.status()->code())
            .bind(":REMARK",     rr.remark())
            .bind(":RAWDATA",    PgCpp::BinaryBindHelper({rr.rawData().data(), rr.rawData().size()}))
            .bind(":EDISESS_ID", rr.ediSession().get())
            .exec();
}

void RemoteResults::updateDb() const
{
    if(PgOra::supportsPg("REMOTE_RESULTS")) {
        updateDbPg(*this);
    } else {
        updateDbOra(*this);
    }
}

static void deleteDbOra(const edilib::EdiSessionId_t &Id)
{
    LogTrace(TRACE7) << __func__ << " by sess_id=" << Id;
#ifdef ENABLE_ORACLE
    Oci8Session os(STDLOG, mainSession());
    const auto query = Delete + " where EDISESSION_ID = :EDISESS_ID";
    Curs8Ctl cur(STDLOG, query, &os);
    cur
          .bind(":EDISESS_ID", Id.get())
          .exec();

    LogTrace(TRACE3) << __func__ << ": " << cur.rowcount() << " rows deleted.";
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

static void deleteDbPg(const edilib::EdiSessionId_t &Id)
{
    LogTrace(TRACE7) << __func__ << " by sess_id=" << Id;
    const auto query = Delete + " where EDISESSION_ID = :EDISESS_ID";
    PgCpp::CursCtl cur = make_pg_curs(PgCpp::getPgManaged(), query);
    cur
          .bind(":EDISESS_ID", Id.get())
          .exec();
    LogTrace(TRACE3) << __func__ << ": " << cur.rowcount() << " rows deleted.";
}

void RemoteResults::deleteDb(const edilib::EdiSessionId_t &Id)
{
    if(PgOra::supportsPg("REMOTE_RESULTS")) {
        deleteDbPg(Id);
    } else {
        deleteDbOra(Id);
    }
}

void cleanOldRecordsOra(const int min_ago)
{
    LogTrace(TRACE7) << __func__;
#ifdef ENABLE_ORACLE
    const auto amin_ago = Dates::second_clock::local_time() - Dates::minutes(min_ago);
    const auto query = Delete + " where DATE_CR < :min_ago";
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG, query, &os);
    cur
            .bind(":min_ago", amin_ago)
            .exec();
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

void cleanOldRecordsPg(const int min_ago)
{
    LogTrace(TRACE7) << __func__;
    const auto amin_ago = Dates::second_clock::local_time() - Dates::minutes(min_ago);
    const auto query = Delete + " where DATE_CR < :min_ago";
    PgCpp::CursCtl cur = make_pg_curs(PgCpp::getPgManaged(), query);
    cur
            .bind(":min_ago", amin_ago)
            .exec();
}

void RemoteResults::cleanOldRecords(const int min_ago)
{
    if(PgOra::supportsPg("REMOTE_RESULTS")) {
        cleanOldRecordsPg(min_ago);
    } else {
        cleanOldRecordsOra(min_ago);
    }
}

bool RemoteResults::isSystemPult() const
{
  return Pult.empty() || Pult == "IATCIP";
}

std::ostream & operator <<(std::ostream & os, const RemoteResults & rr)
{
    os << "  IntMsgId: " << rr.msgId() <<
          "; Pult: " << rr.pult() <<
          "; EdiSess: " << rr.ediSession() <<
          "; RemoteSys: " << rr.remoteSystem() <<
          "; Status: " << rr.status() <<
          "; DateCr: " << rr.dateCr() <<
          "\nErrCode: " << rr.ediErrCode() <<
          "; TLG: " << rr.tlgSource();

    return os;
}

}//namespace edifact
