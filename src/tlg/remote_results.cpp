#include "remote_results.h"
#include "exceptions.h"
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
                             "�訡�� ��ࠡ�⪨ � 㤠������ ���"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::Contrl,
                             "CL",
                             "Communication error (CONTRL)",
                             "�訡�� �裡 (CONTRL)"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::Timeout,
                             "TO",
                             "Time out",
                             "Time out"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::Success,
                             "SS",
                             "Success",
                             "�ᯥ譮"));
    addElem(VTypes,
            RemoteStatusElem(RemoteStatus::RequestSent,
                             "RS",
                             "Request was sent",
                             "����� ��ࠡ��뢠���� 㤠����� ��⮬"));
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace edifact {

using namespace OciCpp;

struct defsupport
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

/////////////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_ORACLE
namespace
{

const std::string select =
        "select INTMSGID, PULT, STATUS, DATE_CR, EDISESSION_ID, REMOTE_ID, "
        "REMARK, EDIERRCODE, RAWDATA from REMOTE_RESULTS";

inline void OciDef(Curs8Ctl &cur, defsupport &def)
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

} //namespace
#endif //ENABLE_ORACLE

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
    //tmp.removeOld(); ���ਥ�����
    tmp.writeDb();
}

/*
��� ����� ���ਥ����� ����뢠���� �� ����, ⮫쪮 �� msgid ���� �� ��. edifact ��ᨨ
��-�� ⮣� �� �� ����� ������ ���� ����� �����६���� ࠡ���� ����� ������ᨬ�� edifact ����ᮢ!
*/
void RemoteResults::readDb(/*const std::string &pult,*/ std::list<RemoteResults> &lres)
{
#ifdef ENABLE_ORACLE
    defsupport defs;

    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
                 select + " where INTMSGID=:INTID", &os);

    auto msgIdAsStr = ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString();

    cur
            .bind(":INTID", msgIdAsStr);
    OciDef(cur, defs);
    cur.exec();

    while(!cur.fen()) {
        lres.push_back(defs.make());
    }

    if(!lres.empty())
    {
        Curs8Ctl(STDLOG,
                 "delete from REMOTE_RESULTS where INTMSGID=:INTID", &os)
                .bind(":INTID", msgIdAsStr)
                .exec();
    }
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}
/*
��� ����� ���ਥ����� ����뢠���� �� ����, ⮫쪮 �� msgid ���� �� ��. edifact ��ᨨ
��-�� ⮣� �� �� ����� ������ ���� ����� �����६���� ࠡ���� ����� ������ᨬ�� edifact ����ᮢ!
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

pRemoteResults RemoteResults::readDb(const edilib::EdiSessionId_t &Id)
{
#ifdef ENABLE_ORACLE
    defsupport defs;
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
                 select + " where EDISESSION_ID = :SID", &os);

    cur
            .bind(":SID", Id.get());

    OciDef(cur, defs);
    cur.EXfet();

    if(cur.rowcount() == 0) {
        return pRemoteResults();
    } else {
        return pRemoteResults(new RemoteResults(defs.make()));
    }
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

void RemoteResults::writeDb()
{
#ifdef ENABLE_ORACLE
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
            "insert into REMOTE_RESULTS "
            "(INTMSGID, PULT, STATUS, DATE_CR, EDISESSION_ID, REMOTE_ID) "
            "values "
            "(:INTMSGID, :PULT, :STATUS, :LOCAL_TIME, :EDISESSION_ID, :REMOTE_ID)", &os);

    cur
            .bind(":INTMSGID",      msgId())
            .bind(":PULT",          pult())
            .bind(":STATUS",        status()->code())
            .bind(":LOCAL_TIME",    Dates::second_clock::local_time())
            .bind(":EDISESSION_ID", ediSession().get())
            .bind(":REMOTE_ID",     remoteSystem().get())
            .exec();
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

void RemoteResults::updateDb() const
{
#ifdef ENABLE_ORACLE
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
            "update REMOTE_RESULTS set "
            "EDIERRCODE = :EDIERRCODE,"
            "STATUS = :STATUS,"
            "REMARK = :REMARK,"
            "RAWDATA = :RAWDATA "
            "where EDISESSION_ID = :EDISESSION_ID", &os);

    cur
            .bind(":EDIERRCODE",    ediErrCode())
            .bind(":STATUS",        status()->code())
            .bind(":REMARK",        remark())
            .bindBlob(":RAWDATA",   rawData())
            .bind(":EDISESSION_ID", ediSession().get())
            .exec();
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

/*
��� ����� ���ਥ����� ����뢠���� �� ����, ⮫쪮 �� msgid ���� �� ��. edifact ��ᨨ
��-�� ⮣� �� �� ����� ������ ���� ����� �����६���� ࠡ���� ����� ������ᨬ�� edifact ����ᮢ!
void RemoteResults::removeOld() const
{
    CursCtl cur = make_curs("delete from REMOTE_RESULTS where INTMSGID <> :INTID "
            "and PULT = :PULT");
    cur
            .bind(":INTID", ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString())
            .bind(":PULT",  pult())
            .exec();

    LogTrace(TRACE3) << __FUNCTION__ << ": " << cur.rowcount() << " rows deleted.";
}
*/

void RemoteResults::deleteDb(const edilib::EdiSessionId_t &Id)
{
#ifdef ENABLE_ORACLE
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
                 "delete from REMOTE_RESULTS where EDISESSION_ID = :SID", &os);
    cur
          .bind(":SID", Id.get())
          .exec();

    LogTrace(TRACE3) << __FUNCTION__ << ": " << cur.rowcount() << " rows deleted.";
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

void RemoteResults::cleanOldRecords(const int min_ago)
{
#ifdef ENABLE_ORACLE
    using namespace Dates;
    LogTrace(TRACE3) << __FUNCTION__;
    const auto amin_ago = second_clock::local_time() - minutes(min_ago);
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
                 "delete from REMOTE_RESULTS where DATE_CR < :min_ago", &os);
    cur
            .bind(":min_ago", amin_ago)
            .exec();
#else
    throw EXCEPTIONS::Exception("Oracle not enabled");
#endif
}

bool RemoteResults::isSystemPult() const
{
  return Pult.empty() || Pult == "IATCIP";
}

std::ostream & operator <<(std::ostream & os, const RemoteResults & rr)
{
    os << "  Pult: " << rr.pult() <<
          "; EdiSess: " << rr.ediSession() <<
          "; RemoteSys: " << rr.remoteSystem() <<
          "; Status: " << rr.status() <<
          "; DateCr: " << rr.dateCr() <<
          "\nErrCode: " << rr.ediErrCode() <<
          "; TLG: " << rr.tlgSource();

    return os;
}

}//namespace edifact
