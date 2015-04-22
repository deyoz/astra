/*
*  C++ Implementation: remote_system_context
*
* Description: Контекст системы, от которой пришел запрос
* В которую посылаем запрос
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /*HAVE_CONFIG_H*/

#include "remote_system_context.h"
#include "EdifactProfile.h"
#include "CheckinBaseTypesOci.h"
#include "exceptions.h"
#include "edi_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"

#include <serverlib/posthooks.h>
#include <serverlib/cursctl.h>
#include <etick/etick_msg.h>
#include <etick/exceptions.h>

#define NICKNAME "ANTON"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>


namespace Ticketing {

namespace RemoteSystemContext {

using namespace Ticketing;

UnknownSystAddrs::UnknownSystAddrs(const std::string& src, const std::string& dest)
    : Exception("Invalid addresses pair src:["+src+"], dest:["+dest+"]"), Src(src), Dest(dest)
{
}


boost::shared_ptr<SystemContext> SystemContext::SysCtxt;

void SystemContext::checkContinuity() const
{
    if(OurAddrEdifact.empty() || RemoteAddrEdifact.empty())
    {
        throw EXCEPTIONS::Exception("SystemContext: check continuity failed!");
    }
}

Ticketing::SystemAddrs_t SystemContext::getNextId()
{
    int val = 0;
    OciCpp::CursCtl cur = make_curs("select SYST_ADDRS_SEQ.nextval from dual");
    cur.def(val).EXfet();

    return Ticketing::SystemAddrs_t(val);
}

const SystemContext& SystemContext::Instance(const char *nick, const char *file, unsigned line)
{
    if(!initialized())
    {
        throw TickExceptions::tick_fatal_except(nick, file, line, EtErr::ProgErr,
                                                "SystemContext::Instance is NULL!");
    }
    return *SystemContext::SysCtxt;
}

void SystemContext::deleteDb()
{
    // TODO
}

void SystemContext::addDb()
{
    // TODO
}

void SystemContext::updateDb()
{
    // TODO
}

SystemContext* SystemContext::init(const SystemContext& new_ctxt)
{
    ProgTrace(TRACE0, "Init SystemContext::SysCtxt");
    if(SystemContext::SysCtxt)
    {
        LogError(STDLOG) << "SystemContext::SysCtxt is not null in SystemContext::init function";
        SystemContext::free();
    }
    SystemContext::SysCtxt.reset(new SystemContext(new_ctxt));
    registerHookAfter(SystemContext::free);
    return SystemContext::SysCtxt.get();
}


SystemContext * SystemContext::initDummyContext()
{
    SystemContext dummyCtxt;
    return SystemContext::init(dummyCtxt);
}

bool SystemContext::initialized()
{
    return SystemContext::SysCtxt.get() != NULL;
}

void SystemContext::free()
{
    if(!SystemContext::SysCtxt.get())
    {
        throw TickExceptions::tick_fatal_except(STDLOG, "EtErr::ProgErr",
                                                "SystemContext::Instance is NULL!");
    }
    ProgTrace(TRACE0, "Free SystemContext::SysCtxt");
    SystemContext::SysCtxt.reset();
}

std::string SystemContext::routerCanonName() const
{
    return AstraEdifact::get_canon_name(RemoteAddrEdifact);
}

unsigned SystemContext::edifactResponseTimeOut() const
{
    return 20;
}

// ================== E D S =====================

EdsSystemContext* EdsSystemContext::read(const std::string& airl, const Ticketing::FlightNum_t& flNum)
{
    int systemId = 0;
    std::pair<std::string, std::string> addrs;
    if(!AstraEdifact::get_et_addr_set(airl, flNum?flNum.get():ASTRA::NoExists, addrs, systemId))
        throw system_not_found(airl, flNum);

    SystemContextMaker ctxtMaker;
    ctxtMaker.setIda(Ticketing::SystemAddrs_t(systemId));
    ctxtMaker.setAirline(airl);
    ctxtMaker.setRemoteAddrEdifact(addrs.first);
    ctxtMaker.setOurAddrEdifact(addrs.second);

    return new EdsSystemContext(ctxtMaker.getSystemContext());
}

EdsSystemContext::EdsSystemContext(const SystemContext& baseCnt,
                                   const EdsSystemSettings& sett)
    : SystemContext(baseCnt),
      Settings(EdsSystemSettings(baseCnt.commonSettings(), sett))
{
}


#ifdef XP_TESTING
EdsSystemContext* EdsSystemContext::create4TestsOnly(const std::string& airline,
                                                     const std::string& ediAddr,
                                                     const std::string& ourEdiAddr)
{
    EdsSystemContext* eds = 0;
    try
    {
        eds = read(airline, Ticketing::FlightNum_t());
        eds->deleteDb();
        throw system_not_found(airline, Ticketing::FlightNum_t());
    }
    catch(const system_not_found& e)
    {
        SystemContextMaker ctxtMaker;
        ctxtMaker.setAirline(airline);
        ctxtMaker.setRemoteAddrEdifact(ediAddr);
        ctxtMaker.setOurAddrEdifact(ourEdiAddr);

        eds = new EdsSystemContext(ctxtMaker.getSystemContext());
        eds->addDb();
    }

    return eds;
}
#endif/*XP_TESTING*/


void EdsSystemContext::deleteDb()
{
    std::string sql =
"  delete from ET_ADDR_SET "
"  where ID = :id ";

    int systemId = ida().get();

    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":id", systemId)
       .exec();

    SystemContext::deleteDb();
}

void EdsSystemContext::addDb()
{
    std::string sql =
"  insert into ET_ADDR_SET "
"  (AIRLINE, EDI_ADDR, EDI_OWN_ADDR, ID) "
"  values "
"  (:airline, :edi_addr, :edi_own_addr, :id) ";

    int systemId = getNextId().get();
    std::string airl = airline();
    std::string ediAddr = remoteAddrEdifact();
    std::string ourEdiAddr = ourAddrEdifact();

    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":airline", airl)
       .bind(":edi_addr", ediAddr)
       .bind(":edi_own_addr", ourEdiAddr)
       .bind(":id", systemId)
       .exec();

    if(cur.err() == CERR_DUPK)
    {
        throw DuplicateRecord();
    }

    SystemContext::addDb();
}

void EdsSystemContext::updateDb()
{
    std::string sql =
"  update ET_ADDR_SET set "
"  AIRLINE = :airline, "
"  EDI_ADDR = :edi_addr, "
"  EDI_OWN_ADDR = :edi_own_addr "
"  where ID = :id ";

    int systemId = ida().get();
    std::string airl = airline();
    std::string ediAddr = remoteAddrEdifact();
    std::string ourEdiAddr = ourAddrEdifact();

    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":airline", airl)
       .bind(":edi_addr", ediAddr)
       .bind(":edi_own_addr", ourEdiAddr)
       .bind(":id", systemId)
       .exec();

    if(cur.rowcount() != 1)
    {
        throw EXCEPTIONS::ExceptionFmt(STDLOG) <<
                "update ET_ADDR_SET by id = " << ida() <<
                "; " << cur.rowcount() << " rows updated";
    }

    SystemContext::updateDb();
}

// ================== D C S =====================

DcsSystemContext* DcsSystemContext::read(const std::string& airl, const Ticketing::FlightNum_t& flNum)
{
    std::string sql =
"  select ID, AIRLINE, EDI_ADDR, EDI_OWN_ADDR  "
"  FROM DCS_SYSTEMS  "
"  WHERE AIRLINE = :airl and (FLT_NO is null or FLT_NO = :flt_no) ";

    int systemId = 0;
    std::string airline, ediAddr, ourEdiAddr;
    short null = -1, nnull = 0;

    OciCpp::CursCtl cur = make_curs(sql);
    cur.def(systemId)
       .def(airline)
       .def(ediAddr)
       .def(ourEdiAddr);
    cur.bind(":airl", airl)
       .bind(":flt_no", flNum?flNum.get():0, flNum?&nnull:&null);
    cur.EXfet();


    if(cur.err() == NO_DATA_FOUND)
    {
        LogTrace(TRACE0) << "DCS system not found by airline "
                         << airl << " and flight " << flNum;
        throw system_not_found(airl, flNum);
    }

    SystemContextMaker ctxtMaker;
    ctxtMaker.setIda(Ticketing::SystemAddrs_t(systemId));
    ctxtMaker.setAirline(airline);
    ctxtMaker.setRemoteAddrEdifact(ediAddr);    // their address
    ctxtMaker.setOurAddrEdifact(ourEdiAddr);    // our address
    return new DcsSystemContext(ctxtMaker.getSystemContext());
}

DcsSystemContext::DcsSystemContext(const SystemContext& baseCnt)
    : SystemContext(baseCnt)
{
}

#ifdef XP_TESTING
DcsSystemContext* DcsSystemContext::create4TestsOnly(const std::string& airline,
                                                     const std::string& ediAddr,
                                                     const std::string& ourEdiAddr)
{
    DcsSystemContext* dcs = 0;
    try
    {
        dcs = read(airline, Ticketing::FlightNum_t());
        dcs->deleteDb();
        throw system_not_found(airline, Ticketing::FlightNum_t());
    }
    catch(const system_not_found& e)
    {
        SystemContextMaker ctxtMaker;
        ctxtMaker.setAirline(airline);
        ctxtMaker.setRemoteAddrEdifact(ediAddr);
        ctxtMaker.setOurAddrEdifact(ourEdiAddr);

        dcs = new DcsSystemContext(ctxtMaker.getSystemContext());
        dcs->addDb();
    }

    return dcs;
}
#endif /*XP_TESTING*/

void DcsSystemContext::deleteDb()
{
    SystemContext::deleteDb();
}

void DcsSystemContext::addDb()
{
    std::string sql =
"  insert into DCS_SYSTEMS "
"  (AIRLINE, EDI_ADDR, EDI_OWN_ADDR, ID) "
"  values "
"  (:airline, :edi_addr, :edi_own_addr, :id) ";

    int systemId = getNextId().get();
    std::string airl = airline();
    std::string ediAddr = remoteAddrEdifact();
    std::string ourEdiAddr = ourAddrEdifact();

    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":airline", airl)
       .bind(":edi_addr", ediAddr)
       .bind(":edi_own_addr", ourEdiAddr)
       .bind(":id", systemId)
       .exec();

    if(cur.err() == CERR_DUPK)
    {
        throw DuplicateRecord();
    }

    SystemContext::addDb();
}

void DcsSystemContext::updateDb()
{
    SystemContext::updateDb();
}

//---------------------------------------------------------------------------------------

void SystemContextMaker::setOurAddrEdifact(const std::string &val)
{
    cont.OurAddrEdifact = val;
}

void SystemContextMaker::setRemoteAddrEdifact(const std::string &val)
{
    cont.RemoteAddrEdifact = val;
}

void SystemContextMaker::setIda(Ticketing::SystemAddrs_t val)
{
    cont.Ida = val;
}

void SystemContextMaker::setAirline(const std::string& val)
{
    cont.Airline = val;
}

void SystemContextMaker::setSystemSettings(const SystemSettings& val)
{
    cont.CommonSettings = val;
}

SystemContext SystemContextMaker::getSystemContext()
{
    cont.checkContinuity();
    return cont;
}

}//namespace RemoteSystemContext

}//namespace Ticketing
