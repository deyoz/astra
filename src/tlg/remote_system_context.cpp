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
#include "CheckinBaseTypesOci.h"
#include "exceptions.h"
#include "edi_utils.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "basetables.h"

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
    if(OurAddrEdifact.empty() || RemoteAddrEdifact.empty() || CanonName.empty())
    {
        LogTrace(TRACE0) << "our_addr: " << OurAddrEdifact << "; "
                         << "rem_addr: " << RemoteAddrEdifact << "; "
                         << "canon_name: " << CanonName;
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

void SystemContext::readEdifactProfile()
{
    using edifact::EdifactProfile;
    if(EdifactProfileName.empty()) {
        LogTrace(TRACE0) << "EdifactProfile not set. Use default!";
        EdiProfile.reset(new EdifactProfile(EdifactProfile::createDefault()));
    } else {
        EdiProfile.reset(new EdifactProfile(EdifactProfile::readByName(EdifactProfileName)));
    }
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
    ProgTrace(TRACE0, "SystemContext::addDb");
    OciCpp::CursCtl cur = make_curs(
"insert into EDI_ADDRS (ADDR, CANON_NAME, CFG_ID) "
"values (:addr, :canon_name, :cfg)");
    cur.stb()
       .bind(":addr", remoteAddrEdifact())
       .bind(":canon_name", routerCanonName())
       .bind(":cfg", ida().get());
    cur.exec();
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
    LogTrace(TRACE3) << "initDummyContext";
    SystemContext dummyCtxt = {};
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

unsigned SystemContext::edifactResponseTimeOut() const
{
    return BaseTables::Router(routerCanonName())->resp_timeout();
}

edifact::EdifactProfile SystemContext::edifactProfile() const
{
    ASSERT(EdiProfile);
    return *EdiProfile.get();
}

// ================== E D S =====================

EdsSystemContext* EdsSystemContext::read(const std::string& airl, const Ticketing::FlightNum_t& flNum)
{
    int systemId = 0;
    std::pair<std::string, std::string> addrs;
    if(!AstraEdifact::get_et_addr_set(airl, flNum?flNum.get():ASTRA::NoExists, addrs, systemId)) {
        tst();
        throw system_not_found(airl, flNum);
    }

    SystemContextMaker ctxtMaker;
    ctxtMaker.setIda(Ticketing::SystemAddrs_t(systemId));
    ctxtMaker.setCanonName(AstraEdifact::get_canon_name(addrs.first));
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
                                                     const std::string& ourEdiAddr,
                                                     const std::string& h2hAddr,
                                                     const std::string& ourH2hAddr)
{
    tst();
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
        ctxtMaker.setIda(getNextId());
        RotParams rotParams("MOWET");
        if(!h2hAddr.empty() && !ourH2hAddr.empty()) {
            rotParams.setH2hAddrs(h2hAddr, ourH2hAddr);
        }
        ctxtMaker.setCanonName(createRot(rotParams));
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
    SystemContext &cont = *this;
    std::string sql =
"  insert into ET_ADDR_SET "
"  (AIRLINE, EDI_ADDR, EDI_OWN_ADDR, ID) "
"  values "
"  (:airline, :edi_addr, :edi_own_addr, :id) ";

    int systemId = cont.ida().get();
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
"select ID, AIRLINE, EDI_ADDR, EDI_OWN_ADDR, AIRIMP_ADDR, AIRIMP_OWN_ADDR, EDIFACT_PROFILE, "
"       DECODE(AIRLINE,NULL,0,2)+ "
"       DECODE(FLT_NO,NULL,0,1) AS priority "
"from DCS_ADDR_SET "
"WHERE AIRLINE=:airl AND "
"      (FLT_NO is null OR FLT_NO=:flt_no) "
"ORDER BY priority DESC";    
        
    int systemId = 0;
    std::string airline;
    std::string ediAddr, ourEdiAddr;
    std::string airAddr, ourAirAddr; 
    std::string ediProfileName;
    short null = -1, nnull = 0;

    OciCpp::CursCtl cur = make_curs(sql);
    cur.def(systemId)
       .def(airline)
       .def(ediAddr)
       .def(ourEdiAddr)
       .defNull(airAddr, "")
       .defNull(ourAirAddr, "")
       .defNull(ediProfileName, "");
    cur.bind(":airl", airl)
       .bind(":flt_no", flNum?flNum.get():0, flNum?&nnull:&null);
    cur.EXfet();


    if(cur.err() == NO_DATA_FOUND)
    {
        LogTrace(TRACE0) << "DCS system not found by airline "
                         << airl << " and flight " << flNum;
        throw system_not_found(airl, flNum);
    }

    LogTrace(TRACE3) << "airimp address: " << airAddr;
    LogTrace(TRACE3) << "our airimp address: " << ourAirAddr;

    SystemContextMaker ctxtMaker;
    ctxtMaker.setIda(Ticketing::SystemAddrs_t(systemId));
    ctxtMaker.setCanonName(AstraEdifact::get_canon_name(ediAddr));
    ctxtMaker.setEdifactProfileName(ediProfileName);
    ctxtMaker.setAirline(airline);
    ctxtMaker.setRemoteAddrEdifact(ediAddr);    // their EDIFACT address
    ctxtMaker.setOurAddrEdifact(ourEdiAddr);    // our EDIFACT address
    ctxtMaker.setRemoteAddrAirimp(airAddr);      // their AIRIMP address
    ctxtMaker.setOurAddrAirimp(ourAirAddr);     // our AIRIMP address
    return new DcsSystemContext(ctxtMaker.getSystemContext());
}

DcsSystemContext::DcsSystemContext(const SystemContext& baseCnt)
    : SystemContext(baseCnt)
{
}

#ifdef XP_TESTING
DcsSystemContext* DcsSystemContext::create4TestsOnly(const std::string& airline,
                                                     const std::string& ediAddr,
                                                     const std::string& ourEdiAddr,
                                                     const std::string& airAddr,
                                                     const std::string& ourAirAddr,
                                                     const std::string& h2hAddr,
                                                     const std::string& ourH2hAddr)
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
        ctxtMaker.setIda(getNextId());
        ctxtMaker.setAirline(airline);
        RotParams rotParams("REMDC");
        if(!h2hAddr.empty() && !ourH2hAddr.empty()) {
            rotParams.setH2hAddrs(h2hAddr, ourH2hAddr);
        }
        ctxtMaker.setCanonName(createRot(rotParams));
        ctxtMaker.setRemoteAddrEdifact(ediAddr);
        ctxtMaker.setOurAddrEdifact(ourEdiAddr);
        ctxtMaker.setRemoteAddrAirimp(airAddr);
        ctxtMaker.setOurAddrAirimp(ourAirAddr);
        ctxtMaker.setEdifactProfileName(createIatciEdifactProfile());

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
"  insert into DCS_ADDR_SET "
"  (AIRLINE, EDI_ADDR, EDI_OWN_ADDR, AIRIMP_ADDR, AIRIMP_OWN_ADDR, EDIFACT_PROFILE, ID) "
"  values "
"  (:airline, :edi_addr, :edi_own_addr, :air_addr, :air_own_addr, :edifact_profile, :id) ";

    OciCpp::CursCtl cur = make_curs(sql);
    cur.stb()
       .bind(":airline",         airline())
       .bind(":edi_addr",        remoteAddrEdifact())
       .bind(":edi_own_addr",    ourAddrEdifact())
       .bind(":air_addr",        remoteAddrAirimp())
       .bind(":air_own_addr",    ourAddrAirimp())
       .bind(":edifact_profile", edifactProfileName())
       .bind(":id",              ida().get())
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

iatci::IatciSettings DcsSystemContext::iatciSettings() const
{
    return iatci::readIatciSettings(ida(), true);
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

void SystemContextMaker::setOurAddrAirimp(const std::string& val)
{
    cont.OurAddrAirimp = val;
}

void SystemContextMaker::setRemoteAddrAirimp(const std::string& val)
{
    cont.RemoteAddrAirimp = val;
}

void SystemContextMaker::setIda(Ticketing::SystemAddrs_t val)
{
    cont.Ida = val;
}

void SystemContextMaker::setCanonName(const std::string& val)
{
    cont.CanonName = val;
}

void SystemContextMaker::setEdifactProfileName(const std::string& edifactProfileName)
{
    cont.EdifactProfileName = edifactProfileName;
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
    cont.readEdifactProfile();
    return cont;
}

}//namespace RemoteSystemContext


#ifdef XP_TESTING

std::string createRot(const RotParams &par)
{
    Ticketing::RouterId_t rot;

    OciCpp::CursCtl cur = make_curs(
"select ID from ROT where CANON_NAME=:cn");
    cur.bind(":cn", par.canon_name)
       .def(rot)
       .exfet();

    if(!rot)
    {
        LogTrace(TRACE3) << "create rot: " << par.canon_name;

        make_curs("begin "
"insert into ROT (CANON_NAME, OWN_CANON_NAME, ID, LOOPBACK, IP_ADDRESS, IP_PORT, H2H, H2H_ADDR, OUR_H2H_ADDR) "
"values "
"(:canon_name, 'ASTRA', id__seq.nextval, 1, '0.0.0.0', '8888', :h2h, :h2h_addr, :our_h2h_addr) "
"returning id into :id; end;").
        bind(":canon_name",   par.canon_name).
        bind(":h2h",          par.h2h?1:0).
        bind(":h2h_addr",     par.h2h_addr).
        bind(":our_h2h_addr", par.our_h2h_addr).
        bindOut(":id",        rot).
        exec();

        BaseTables::CacheData<BaseTables::Router_impl>::clearCache();

        LogTrace(TRACE3) << "rot.ida = " << rot << " for canon_name = " << par.canon_name;
    }
    return par.canon_name;
}

std::string createIatciEdifactProfile()
{
    edifact::EdifactProfileData p("IATCI",
                                  "94",
                                  "1",
                                  "IA",
                                  "IATA",
                                  1);

    make_curs("delete from EDIFACT_PROFILES where NAME=:name")
            .bind(":name", p.m_profileName)
            .exec();

    LogTrace(TRACE3) << "create edifact profile: " << p.m_profileName;

    make_curs(
"insert into EDIFACT_PROFILES (NAME, VERSION, SUB_VERSION, CTRL_AGENCY, SYNTAX_NAME, SYNTAX_VER) "
"values "
"(:name, :version, :sub_version, :ctrl_agency, :syntax_name, :syntax_ver)")
            .bind(":name",        p.m_profileName)
            .bind(":version",     p.m_version)
            .bind(":sub_version", p.m_subVersion)
            .bind(":ctrl_agency", p.m_ctrlAgency)
            .bind(":syntax_name", p.m_syntaxName)
            .bind(":syntax_ver",  p.m_syntaxVer)
            .exec();

    return p.m_profileName;
}

#endif//XP_TESTING

}//namespace Ticketing
