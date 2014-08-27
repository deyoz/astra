/*
*  C++ Implementation: RemoteResults
*
* Description: remote request results
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
*
*/
#include "remote_results.h"
#include "exceptions.h"
#include "astra_dates_oci.h"
#include <serverlib/cursctl.h>
#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>
#include <serverlib/int_parameters_oci.h>
#include <tclmon/internal_msgid.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

const char *edifact::RemoteStatusElem::ElemName = "Remote action status";

using namespace edifact;
template <> BaseTypeElemHolder< RemoteStatusElem >::TypesMap
        BaseTypeElemHolder< RemoteStatusElem >::VTypes =
        BaseTypeElemHolder< RemoteStatusElem >::TypesMap();
template <> bool BaseTypeElemHolder<RemoteStatusElem>::initialized = false;
template <> void BaseTypeElemHolder<RemoteStatusElem>::init()
{
    addElem( RemoteStatusElem(RemoteStatus::CommonError,
             "CE",
             "Error in remote host", "Ошибка обработки в удаленном хосте"));
    addElem( RemoteStatusElem(RemoteStatus::Contrl,
             "CL",
             "Communication error (CONTRL)",  "Ошибка связи (CONTRL)"));
    addElem( RemoteStatusElem(RemoteStatus::Timeout,
             "TO",
             "Time out",  "Time out"));
    addElem( RemoteStatusElem(RemoteStatus::Success,
             "SS",
             "Success",  "Успешно"));
    addElem( RemoteStatusElem(RemoteStatus::RequestSent,
             "RS",
             "Request was sent",  "Запрос обрабатывается удаленным хостом"));
}


namespace edifact
{
using namespace OciCpp;

namespace
{
    const std::string select =
            "select PULT, STATUS, DATE_CR, EDISESSION_ID, REMOTE_ID, "
            "REMARK, EDIERRCODE, TLG_TEXT from REMOTE_RESULTS";
}

struct defsupport
{
    std::string Pult;
    std::string Status;
    edilib::EdiSessionId_t EdiSession;
    Ticketing::SystemAddrs_t  RemoteSystem;
    std::string Remark;
    std::string EdiErrCode;
    oracle_datetime odt;
    std::string TlgSource;

    RemoteResults make()
    {
        using namespace Dates;
        RemoteResults rr;
        rr.DateCr = from_oracle_time(odt);
        rr.Status = RemoteStatus(Status);
        rr.Pult = Pult;
        rr.EdiSession = EdiSession;
        rr.RemoteSystem = RemoteSystem;
        rr.Remark = Remark;
        rr.EdiErrCode = EdiErrCode;
        rr.TlgSource = TlgSource;

        return rr;
    }
};

inline void OciDef(CursCtl &ctl, defsupport &def)
{
    ctl.
            def(def.Pult).
            def(def.Status).
            def(def.odt).
            def(def.EdiSession).
            def(def.RemoteSystem).
            defNull(def.Remark, "").
            defNull(def.EdiErrCode, "").
            defNull(def.TlgSource, "");
}

RemoteResults::RemoteResults(const std::string &pult,
                             const edilib::EdiSessionId_t &edisess,
                             const Ticketing::SystemAddrs_t &remoteId)
    : Pult(pult),
           EdiSession(edisess),
           Status(RemoteStatus::RequestSent),
           RemoteSystem(remoteId)
{
}

void RemoteResults::add(const std::string &pult,
                        const edilib::EdiSessionId_t &edisess,
                        const Ticketing::SystemAddrs_t &remoteId)
{
    RemoteResults tmp(pult, edisess, remoteId);
    tmp.removeOld();
    tmp.writeDb();
}

void RemoteResults::readDb(const std::string &pult, std::list<RemoteResults> &lres)
{
    defsupport defs;

    CursCtl cur = make_curs((select + " where pult = :pult and INTMSGID=:INTID").c_str());

    cur.
            stb().
            bind(":pult", pult).
            bind(":INTID", ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString());
    OciDef(cur, defs);
    cur.exec();

    while(!cur.fen())
    {
        lres.push_back(defs.make());
    }

    if(!lres.empty())
    {
        make_curs("delete from REMOTE_RESULTS where pult = :p").
                bind(":p", pult).
                exec();
    }
}

pRemoteResults RemoteResults::readSingle(const std::string &pult)
{
    std::list<RemoteResults> lres;
    readDb(pult, lres);
    if(lres.size() != 1)
    {
        LogError(STDLOG) << "Got " << lres.size() << " for " << pult <<
                " MsgId: " << ServerFramework::getQueryRunner().getEdiHelpManager().msgId() <<
                " remote results while one was expected";
        throw EXCEPTIONS::Exception("Failed to read results"); // TODO throw UserException
    }
/*    else if(lres.empty())
    {
        return pRemoteResults();
    }*/
    else
    {
        return pRemoteResults(new RemoteResults(lres.front()));
    }
}

pRemoteResults RemoteResults::readDb(edilib::EdiSessionId_t Id)
{
    defsupport defs;
    CursCtl cur = make_curs((select + " where EDISESSION_ID = :SID").c_str());

    cur.
            bind(":SID", Id);

    OciDef(cur, defs);
    cur.EXfet();

    if(cur.err() == NO_DATA_FOUND)
    {
        return pRemoteResults();
    }
    else
    {
        return pRemoteResults(new RemoteResults(defs.make()));
    }
}

void RemoteResults::writeDb()
{
    CursCtl cur = make_curs("insert into REMOTE_RESULTS "
                            "(INTMSGID, PULT, STATUS, DATE_CR, EDISESSION_ID, REMOTE_ID) "
                            "values "
                            "(:INTMSGID, :PULT, :STATUS, :LOCAL_TIME, :EDISESSION_ID, :REMOTE_ID)");

    cur.
            stb().
            bind(":INTMSGID", ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString()).
            bind(":PULT", pult()).
            bind(":STATUS", status()->code()).
            bind(":LOCAL_TIME", OciCpp::to_oracle_datetime(Dates::second_clock::local_time())).
            bind(":EDISESSION_ID", ediSession()).
            bind(":REMOTE_ID", remoteSystem()).
            exec();
}

void RemoteResults::updateDb() const
{
    CursCtl cur = make_curs(
            "update REMOTE_RESULTS set "
            "EDIERRCODE = :EDIERRCODE,"
            "STATUS = :STATUS,"
            "REMARK = :REMARK,"
            "TLG_TEXT = :TEXT "
            "where EDISESSION_ID = :EDISESSION_ID");

    cur.
            bind(":EDIERRCODE", ediErrCode()).
            bind(":STATUS", status()->code()).
            bind(":REMARK", remark()).
            bind(":TEXT",   tlgSource()).
            bind(":EDISESSION_ID", ediSession()).
            exec();
}

void RemoteResults::removeOld() const
{
    CursCtl cur = make_curs("delete from REMOTE_RESULTS where INTMSGID <> :INTID "
            "and PULT = :PULT");
    cur.
            stb().
            bind(":INTID", ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString()).
            bind(":PULT",  pult()).
            exec();

    LogTrace(TRACE3) << __FUNCTION__ << ": " << cur.rowcount() << " rows deleted.";
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

}//namespace Ticketing
