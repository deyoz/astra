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


boost::shared_ptr<SystemContext> SysCtxt;

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

SystemContext SystemContext::defSelData(OciCpp::CursCtl& cur)
{
    int systemId = 0;
    std::string airline;
    std::string ediAddr, ourEdiAddr;
    std::string ediAddrExt, ourEdiAddrExt;
    std::string airAddr, ourAirAddr;
    std::string ediProfileName;

    cur.def(systemId)
       .def(airline)
       .def(ediAddr)
       .def(ourEdiAddr)
       .defNull(ediAddrExt, "")
       .defNull(ourEdiAddrExt, "")
       .defNull(airAddr, "")
       .defNull(ourAirAddr, "")
       .defNull(ediProfileName, "");
    cur.EXfet();

    if(cur.err() == NO_DATA_FOUND)
    {
        throw system_not_found();
    }

    SystemContextMaker ctxtMaker;
    ctxtMaker.setIda(Ticketing::SystemAddrs_t(systemId));
    ctxtMaker.setCanonName(AstraEdifact::get_canon_name(ediAddr));
    ctxtMaker.setEdifactProfileName(ediProfileName);
    ctxtMaker.setAirline(airline);
    ctxtMaker.setRemoteAddrEdifact(ediAddr);
    ctxtMaker.setOurAddrEdifact(ourEdiAddr);
    ctxtMaker.setRemoteAddrAirimp(airAddr);
    ctxtMaker.setOurAddrAirimp(ourAirAddr);
    ctxtMaker.setRemoteAddrEdifactExt(ediAddrExt);
    ctxtMaker.setOurAddrEdifactExt(ourEdiAddrExt);
    return ctxtMaker.getSystemContext();
}


void SystemContext::readEdifactProfile()
{
    using edifact::EdifactProfile;
    if(EdifactProfileName.empty()) {
        LogTrace(TRACE0) << "EdifactProfile not set. Use default!";
        EdiProfile = std::make_shared<EdifactProfile>(EdifactProfile::createDefault());
    } else {
        EdiProfile = std::make_shared<EdifactProfile>(EdifactProfile::readByName(EdifactProfileName));
    }
}

SystemContext SystemContext::readByEdiAddrs(const std::string &source, const std::string &source_ext,
                                            const std::string &dest,   const std::string &dest_ext)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << source << "/" << source_ext << " and "
                     << dest << "/" << dest_ext;
    std::unique_ptr<SystemContext> dcs(DcsSystemContext::readByEdiAddrs(source, source_ext, dest, dest_ext, false));
    std::unique_ptr<SystemContext> eds(EdsSystemContext::readByEdiAddrs(source, source_ext, dest, dest_ext, false));
    if(!dcs && !eds) {
        throw UnknownSystAddrs(source + "/" + source_ext,
                               dest + "/" + dest_ext);
    }

    if(dcs && eds) {
        throw EXCEPTIONS::ExceptionFmt() <<
                "unknown system type (DCS or EDS?)";
    }

    if(dcs) return *dcs;
    if(eds) return *eds;

    throw EXCEPTIONS::ExceptionFmt() << "Can't be here";
}

const SystemContext& SystemContext::Instance(const char *nick, const char *file, unsigned line)
{
    if(!initialized())
    {
        throw TickExceptions::tick_fatal_except(nick, file, line, EtErr::ProgErr,
                                                "SystemContext::Instance is NULL!");
    }
    return *SysCtxt;
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
    ProgTrace(TRACE0, "Init SysCtxt");
    if(SysCtxt)
    {
        LogError(STDLOG) << "SysCtxt is not null in SystemContext::init function";
        SystemContext::free();
    }
    SysCtxt.reset(new SystemContext(new_ctxt));
    registerHookAfter(SystemContext::free);
    return SysCtxt.get();
}


SystemContext* SystemContext::initDummyContext()
{
    LogTrace(TRACE3) << __FUNCTION__;
    SystemContext dummyCtxt = {};
    return SystemContext::init(dummyCtxt);
}

SystemContext* SystemContext::initEdifact(const std::string& src, const std::string& src_ext,
                                          const std::string& dest,const std::string& dest_ext)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << src << "/" << src_ext << " and "
                     << dest << "/" << dest_ext;
    return SystemContext::init(SystemContext::readByEdiAddrs(src, src_ext, dest, dest_ext));
}

SystemContext* SystemContext::initEdifactByAnswer(const std::string& src, const std::string& src_ext,
                                                  const std::string& dest,const std::string& dest_ext)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << src << "/" << src_ext << " and "
                     << dest << "/" << dest_ext;
    return SystemContext::init(SystemContext::readByEdiAddrs(src, src_ext, dest, dest_ext));
}

bool SystemContext::initialized()
{
    return SysCtxt.get() != NULL;
}

void SystemContext::free()
{
    if(!SysCtxt.get())
    {
        throw TickExceptions::tick_fatal_except(STDLOG, "EtErr::ProgErr",
                                                "SystemContext::Instance is NULL!");
    }
    ProgTrace(TRACE0, "Free SysCtxt");
    SysCtxt.reset();
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

BaseTables::Company SystemContext::airlineImpl() const
{
    return BaseTables::Company(airline());
}

// ================== E D S =====================

EdsSystemContext* EdsSystemContext::read(const std::string& airl, const Ticketing::FlightNum_t& flNum)
{
    std::string sql = getSelectSql();
    sql +=
"WHERE AIRLINE=:airl AND (FLT_NO is null OR FLT_NO=:flt_no) "
"ORDER BY priority DESC";

    short null = -1, nnull = 0;
    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":airl", airl)
       .bind(":flt_no", flNum?flNum.get():0, flNum?&nnull:&null);

    SystemContext sysCtxt;
    try
    {
        sysCtxt = defSelData(cur);
    }
    catch(system_not_found)
    {
        throw system_not_found(airl, flNum);
    }

    return new EdsSystemContext(sysCtxt);
}

SystemContext* EdsSystemContext::readByEdiAddrs(const std::string& source, const std::string& source_ext,
                                                const std::string& dest,   const std::string& dest_ext,
                                                bool throwNf)
{
    std::string sql = getSelectSql();
    sql +=
"where EDI_ADDR = :src and EDI_OWN_ADDR = :dest ";
    sql +=
"and (EDI_ADDR_EXT = :src_ext or :src_ext is null) "
"and (EDI_OWN_ADDR_EXT = :dest_ext or :dest_ext is null) ";
    sql += "ORDER BY priority DESC";

    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":src", source)
       .bind(":src_ext", source_ext)
       .bind(":dest", dest)
       .bind(":dest_ext", dest_ext);

    try
    {
        return new EdsSystemContext(defSelData(cur));
    }
    catch(system_not_found)
    {
        if(throwNf) {
            LogWarning(STDLOG) << "Unknown edifact addresses pair "
                    "[" << source << "::" << source_ext << "]/"
                    "[" << dest   << "::" << dest_ext   << "]";
            throw UnknownSystAddrs(source + "/" + source_ext,
                                   dest + "/" + dest_ext);
        }
    }

    return nullptr;
}

EdsSystemContext::EdsSystemContext(const SystemContext& baseCnt,
                                   const EdsSystemSettings& sett)
    : SystemContext(baseCnt),
      Settings(EdsSystemSettings(baseCnt.commonSettings(), sett))
{
}


#ifdef XP_TESTING
void EdsSystemContext::create4TestsOnly(const std::string& airline,
                                        const std::string& ediAddr,
                                        const std::string& ourEdiAddr,
                                        bool translit,
                                        const std::string& h2hAddr,
                                        const std::string& ourH2hAddr)
{
    std::unique_ptr<EdsSystemContext> eds;
    try
    {
        eds.reset(read(airline,
                       Ticketing::FlightNum_t()));
        eds->deleteDb();
        throw system_not_found(airline, Ticketing::FlightNum_t());
    }
    catch(const system_not_found& e)
    {
        SystemContextMaker ctxtMaker;
        ctxtMaker.setIda(getNextId());
        RotParams rotParams("MOWET");
        rotParams.translit = translit;
        if(!h2hAddr.empty() && !ourH2hAddr.empty()) {
            rotParams.setH2hAddrs(h2hAddr, ourH2hAddr);
        }
        ctxtMaker.setCanonName(createRot(rotParams));
        ctxtMaker.setAirline(airline);
        ctxtMaker.setRemoteAddrEdifact(ediAddr);
        ctxtMaker.setOurAddrEdifact(ourEdiAddr);
        eds.reset(new EdsSystemContext(ctxtMaker.getSystemContext()));
        eds->addDb();
    }
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

std::string EdsSystemContext::getSelectSql()
{
    return
"select ID, AIRLINE, EDI_ADDR, EDI_OWN_ADDR, EDI_ADDR_EXT, EDI_OWN_ADDR_EXT, "
"       AIRIMP_ADDR, AIRIMP_OWN_ADDR, EDIFACT_PROFILE, "
"       DECODE(AIRLINE,NULL,0,2)+ "
"       DECODE(FLT_NO,NULL,0,1) AS priority "
"from ET_ADDR_SET ";
}

// ================== D C S =====================

DcsSystemContext* DcsSystemContext::read(const std::string& airl, const Ticketing::FlightNum_t& flNum)
{
    std::string sql = getSelectSql();
    sql +=
"WHERE AIRLINE=:airl AND (FLT_NO is null OR FLT_NO=:flt_no) "
"ORDER BY priority DESC";

    short null = -1, nnull = 0;
    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":airl", airl)
       .bind(":flt_no", flNum?flNum.get():0, flNum?&nnull:&null);

    try
    {
        return new DcsSystemContext(defSelData(cur));
    }
    catch(system_not_found)
    {
        throw system_not_found(airl, flNum);
    }
}

SystemContext* DcsSystemContext::readByEdiAddrs(const std::string& source, const std::string& source_ext,
                                                const std::string& dest,   const std::string& dest_ext,
                                                bool throwNf)
{
    std::string sql = getSelectSql();
    sql +=
"where EDI_ADDR = :src and EDI_OWN_ADDR = :dest ";
    sql +=
"and (EDI_ADDR_EXT = :src_ext or :src_ext is null) "
"and (EDI_OWN_ADDR_EXT = :dest_ext or :dest_ext is null) ";
    sql += "ORDER BY priority DESC";

    OciCpp::CursCtl cur = make_curs(sql);
    cur.bind(":src", source)
       .bind(":src_ext", source_ext)
       .bind(":dest", dest)
       .bind(":dest_ext", dest_ext);

    try
    {
        return new DcsSystemContext(defSelData(cur));
    }
    catch(system_not_found)
    {
        if(throwNf) {
            LogWarning(STDLOG) << "Unknown edifact addresses pair "
                    "[" << source << "::" << source_ext << "]/"
                    "[" << dest   << "::" << dest_ext   << "]";
            throw UnknownSystAddrs(source + "/" + source_ext,
                                   dest + "/" + dest_ext);
        }
    }

    return nullptr;
}

DcsSystemContext::DcsSystemContext(const SystemContext& baseCnt)
    : SystemContext(baseCnt)
{
}

#ifdef XP_TESTING
void DcsSystemContext::create4TestsOnly(const std::string& airline,
                                        const std::string& ediAddr,
                                        const std::string& ourEdiAddr,
                                        const std::string& airAddr,
                                        const std::string& ourAirAddr,
                                        const std::string& h2hAddr,
                                        const std::string& ourH2hAddr)
{
    std::unique_ptr<DcsSystemContext> dcs;
    try
    {
        dcs.reset(read(airline,
                       Ticketing::FlightNum_t()));
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

        dcs.reset(new DcsSystemContext(ctxtMaker.getSystemContext()));
        dcs->addDb();
    }
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

std::string DcsSystemContext::getSelectSql()
{
    return
"select ID, AIRLINE, EDI_ADDR, EDI_OWN_ADDR, EDI_ADDR_EXT, EDI_OWN_ADDR_EXT, "
"       AIRIMP_ADDR, AIRIMP_OWN_ADDR, EDIFACT_PROFILE, "
"       DECODE(AIRLINE,NULL,0,2)+ "
"       DECODE(FLT_NO,NULL,0,1) AS priority "
"from DCS_ADDR_SET ";
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

void SystemContextMaker::setOurAddrEdifactExt(const std::string& val)
{
    cont.OurAddrEdifactExt = val;
}

void SystemContextMaker::setRemoteAddrEdifactExt(const std::string& val)
{
    cont.RemoteAddrEdifactExt = val;
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
    cont.RemoteAirline = val;
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
"insert into ROT (CANON_NAME, OWN_CANON_NAME, ROUTER_TRANSLIT, ID, LOOPBACK, IP_ADDRESS, IP_PORT, H2H, H2H_ADDR, OUR_H2H_ADDR) "
"values "
"(:canon_name, 'ASTRA', :translit, id__seq.nextval, 1, '0.0.0.0', '8888', :h2h, :h2h_addr, :our_h2h_addr) "
"returning id into :id; end;").
        bind(":canon_name",   par.canon_name).
        bind(":translit",     par.translit).
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
