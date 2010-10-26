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

// #include "tlg_context.h"
// #include "basetables.h"
#include "remote_system_context.h"
#include "EdifactProfile.h"
#include "etick/etick_msg.h"
#include "serverlib/posthooks.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace Ticketing;
//using namespace edifact;
using namespace TickExceptions;
//using namespace OciCpp;

class system_not_found {
};


namespace Ticketing{
template <> BaseTypeElemHolder< SystemTypeElem >::TypesMap
        BaseTypeElemHolder< SystemTypeElem >::VTypes =
        BaseTypeElemHolder< SystemTypeElem >::VTypes;
template <> bool BaseTypeElemHolder<SystemTypeElem>::initialized = false;
const char *SystemTypeElem::ElemName = "Remote System Type";

template <> void Ticketing::BaseTypeElemHolder<SystemTypeElem>::init()
{
    addElem(SystemTypeElem(SystemType::DcsSystem,"DCS",
                           "Система регистрации",
                           "Ground handling system"));
    addElem(SystemTypeElem(SystemType::EtsSystem,"ETS",
                           "Сервер электронных билетов",
                           "Electronic ticket server"));
}
}

string SystemContext::getSelText()
{
    return "";
    /// @TODO VLAD дай мне SQL для базовых полей
/*            "select REMOTE_AIRLINE, "
            " UNB_ADDR,     UNB_ADDR_INT,     UNB_ADDR_SND, "
            " OUR_UNB_ADDR, OUR_UNB_ADDR_INT, OUR_UNB_ADDR_SND, "
            " AIRIMP_ADDR, OUR_AIRIMP_ADDR, AIRLINE,"
            " ROUTER, TYPE, DESCRIPTION, EDIFACT_PROFILE_ID, "
            " DEBUG_INFO, OPEN_RPI, SEND_CRI, IDA"
            " from system_addresses where close = 0";*/
}

void SystemContext::splitEdifactAddrs(std::string src, std::string &dest, std::string &dest_ext)
{
    std::string::size_type pos = src.find_first_of("/");
    if(pos != std::string::npos)
    {
        dest_ext = src.substr(pos+1);
        dest = src.substr(0,pos);
    }
    else
    {
        dest = src;
    }
}

std::string SystemContext::joinEdifactAddrs(const std::string &src, const std::string &src_ext)
{
    if(src_ext.empty())
        return src;
    else
        return src + "/" + src_ext;
}

void SystemContext::defSelData(/*OciCpp::CursCtl &curs, */SystemContext &ctxt)
{
//     curs.
//             def(ctxt.RemoteAirline).
//             def(ctxt.RemoteAddrEdifact).
//             defNull(ctxt.RemoteAddrEdifactExt,"").
//             def(ctxt.RemoteAddrEdifactSnd).
//             def(ctxt.OurAddrEdifact).
//             defNull(ctxt.OurAddrEdifactExt,"").
//             def(ctxt.OurAddrEdifactSnd).
//             def(ctxt.RemoteAddrAirimp).
//             def(ctxt.OurAddrAirimp).
//             def(ctxt.OurAirline).
//             def(ctxt.Router).
//             def(type__).
//             defNull(ctxt.Description,"").
//             def(ctxt.EdifactProfileId).
//             def(DebugInfo).
//             def(OpenRpi).
//             def(SendCri).
//             def(ctxt.Ida).
//             EXfet();

//     if(curs.err() == NO_DATA_FOUND)
//     {
//         throw system_not_found();
//     }
//     if(type__.size()!=1)
//     {
//         throw tick_fatal_except(STDLOG, EtErr::ProgErr,
//                                 "Type in system_addresses is %s", type__.c_str());
//     }
//     ctxt.SysType = SystemType::fromTypeStr(type__.c_str());

//     ctxt = mk.getSystemContext();
//    return curs;
}

SystemContext SystemContext::readByEdiAddrsQR(
        const std::string &source, const std::string &source_ext,
        const std::string &dest,   const std::string &dest_ext,
        bool by_req)
{
    SystemContext ctxt;
    string select_text = SystemContext::getSelText();
    if(by_req)
    {
        select_text += " and UNB_ADDR = :src and OUR_UNB_ADDR = :dst";
    }
    else
    {
        select_text += " and UNB_ADDR_SND = :src and OUR_UNB_ADDR_SND = :dst";
    }

    select_text += " and (UNB_ADDR_INT     = :src_int OR :src_int IS NULL)"
                   " and (OUR_UNB_ADDR_INT = :dst_int OR :dst_int IS NULL)";

//     CursCtl cur = make_curs(select_text.c_str());
/*    cur.
            bind(":src", source).
            bind(":dst", dest).
            bind(":src_int", source_ext).
            bind(":dst_int", dest_ext);*/
    try
    {
        SystemContext::defSelData(/*cur, */ctxt);
    }
    catch(system_not_found)
    {
        LogWarning(STDLOG) << "Unknown edifact addresses pair "
                "[" << source << "::" << source_ext << "]/"
                "[" << dest   << "::" << dest_ext   << "]";
        throw UnknownSystAddrs(joinEdifactAddrs(source,source_ext),
                               joinEdifactAddrs(dest,  dest_ext));
    }

    return ctxt;
}

SystemContext SystemContext::readByEdiAddrs
        (const std::string &source, const std::string &source_ext,
         const std::string &dest,   const std::string &dest_ext)
{
    return readByEdiAddrsQR(source,source_ext,dest,dest_ext,true);
}

SystemContext SystemContext::readByAnswerEdiAddrs(const std::string &source,
        const std::string &source_ext,
        const std::string &dest,
        const std::string &dest_ext)
{
    return readByEdiAddrsQR(source,source_ext,dest,dest_ext,false);
}

boost::shared_ptr<SystemContext> SystemContext::SysCtxt;

const SystemContext &SystemContext::Instance(const char *nick, const char *file, unsigned line)
{
    if(!initialized())
    {
        throw tick_fatal_except(nick, file, line, EtErr::ProgErr,
                                "SystemContext::Instance is NULL!");
    }
    return *SystemContext::SysCtxt;
}

SystemContext * SystemContext::init(const SystemContext &new_ctxt)
{
    if(SystemContext::SysCtxt)
    {
        LogError(STDLOG) << "SystemContext::SysCtxt is not null in SystemContext::init function";
        SystemContext::free();
    }
    SystemContext::SysCtxt.reset( new_ctxt.readChildByType() );
    /// !!!! @TODO VLAD !!! Проверить вызов call post hooks!!!
    registerHookAfter(SystemContext::free);
    return SystemContext::SysCtxt.get();
}

SystemContext * SystemContext::readChildByType() const
{
    switch( systemType()->type() )
    {
        case SystemType::DcsSystem:
            return new DcsSystemContext(DcsSystemContext::readFromDb(*this));
        case SystemType::EtsSystem:
            return new EtsSystemContext(EtsSystemContext::readFromDb(*this));
        default: ;
            throw EXCEPTIONS::ExceptionFmt() <<
                    "unknown system type " << systemType()->code() <<
                    "(" << systemType()->type() << ")";
    }
}

SystemContext * SystemContext::initEdifact(const std::string &src, const std::string &src_ext,
                                           const std::string &dest, const std::string &dest_ext)
{
    return SystemContext::init(SystemContext(SystemContext::readByEdiAddrs(src, src_ext, dest, dest_ext)));
}

SystemContext * SystemContext::initEdifactByAnswer(const std::string &src,
        const std::string &src_ext,
        const std::string &dest,
        const std::string & dest_ext)
{
    return SystemContext::init(SystemContext(SystemContext::readByAnswerEdiAddrs(src, src_ext, dest, dest_ext)));
}

bool SystemContext::initialized()
{
    return SystemContext::SysCtxt;
}

void SystemContext::free()
{
    if(!SystemContext::SysCtxt.get())
    {
        throw tick_fatal_except(STDLOG, EtErr::ProgErr,
                                "SystemContext::Instance is NULL!");
    }
    ProgTrace(TRACE0, "Free SystemContext::SysCtxt");
    SystemContext::SysCtxt.reset();
}

SystemContext SystemContext::readById(ASTRA::SystemAddrs_t Id)
{
    SystemContext system;

    std::string sqltxt = getSelText();
    sqltxt += " and ida = :id";
  //  CursCtl cur = make_curs(sqltxt.c_str());

  //  cur.bind(":id", Id);

    try{
        SystemContext::defSelData(/*cur,*/ system);
    }

    catch(system_not_found)
    {
        throw EXCEPTIONS::ExceptionFmt(STDLOG) << "No such system context by id=" << Id;
    }

    return system;
}

const edifact::EdifactProfile & SystemContext::edifactProfile() const
{
    if(!EdifactProfileCache)
        EdifactProfileCache.reset(new edifact::EdifactProfile(edifact::EdifactProfile::load(edifactProfileId())));

    return *EdifactProfileCache;
}

//================ DcsSystemContext ================
DcsSystemContext DcsSystemContext::readFromDb(const SystemContext & baseCnt)
{
    DcsSystemContext cont(baseCnt);
    std::string regtype;

//     CursCtl cur = make_curs("select AIRPORT, REG_TYPE, SEND_POS from dcs_systems where ida = :id");
//     cur.
//             throwAll().
//             bind(":id", baseCnt.ida()).
//             def(cont.Airport).
//             def(regtype).
//             def(send_pos).
//             EXfet();
//     cont.RegType = RegistrationType::fromTypeStr(regtype.c_str());

    cont.Settings = DcsSystemSettings();

    return cont;
}

DcsSystemContext DcsSystemContext::readById(ASTRA::SystemAddrs_t Id)
{
    return DcsSystemContext::readFromDb(SystemContext::readById(Id));
}

const DcsSystemContext & SystemContext::DcsInstance(const char *nick, const char *file, unsigned line)
{
    if( SystemContext::Instance(nick, file, line).systemType() != SystemType::DcsSystem)
    {
        throw InvalidSystemTypeCast();
    }

    return dynamic_cast<const DcsSystemContext &>(SystemContext::Instance(nick, file, line));
}


// ================== E T S =====================
EtsSystemContext EtsSystemContext::readFromDb(const SystemContext & baseCnt)
{
    EtsSystemContext ets (baseCnt, /*read ETS settings here*/EtsSystemSettings());

    return ets;
}

EtsSystemContext EtsSystemContext::readById(ASTRA::SystemAddrs_t Id)
{
    return EtsSystemContext::readFromDb(SystemContext::readById(Id));
}

const EtsSystemContext & SystemContext::EtsInstance(const char *nick, const char *file, unsigned line)
{
    if( SystemContext::Instance(nick, file, line).systemType() != SystemType::EtsSystem)
    {
        throw InvalidSystemTypeCast();
    }

    return dynamic_cast<const EtsSystemContext &>(SystemContext::Instance(nick, file, line));
}

EtsSystemContext::EtsSystemContext(const SystemContext & baseCnt,
                                   const EtsSystemSettings & sett)
    :SystemContext(baseCnt),
     Settings(EtsSystemSettings(baseCnt.commonSettings(), sett))
{
}

UnknownSystAddrs::UnknownSystAddrs(const std::string &src, const std::string &dest)
    :Exception("Invalid addresses pair src:["+src+"], dest:["+dest+"]"),
               Src(src), Dest(dest)
{
}

unsigned SystemContext::edifactResponseTimeOut() const
{
    return 20; // TODO VLAD - return timeout here
}

//}  namespace edifact

#ifdef XP_TESTING11111
#include "xp_testing.h"
using namespace Ticketing;
using namespace Ticketing::RemoteSystemContext;
namespace {
    void init()
    {
        testInitDB();
    }

    void tear_down()
    {
        testClearShutDB();
    }
}

START_TEST(chk_ets_cfg_work)
{
    try
    {
        // Проверка на создание
        SystemContextMaker mk;
        mk.setRouter(RouterId_t(1));
        mk.setSysType(EtsSystem);
        mk.setOurAddrAirimp("OURADDR");
        mk.setRemoteAddrAirimp("REMADDR");
        mk.setOurAddrEdifact("OURFACT");
        mk.setRemoteAddrEdifact("REMFACT");
        mk.setOurAirline(BaseTables::Company("UT")->ida());
        mk.setRemoteAirline(BaseTables::Company("H8")->ida());
        mk.setEdifactProfileId(edifact::EdifactProfile::idByName("SIRENA-ETS"));
        EtsSystemSettings sett;
        sett.setAskForControlStatus(CouponStatus(CouponStatus::Airport));
        sett.setSendSeparatedCos(true);
        sett.setPushControlTime(Dates::time_duration(Dates::hours(72)));
        sett.setXtTax(true);
        EtsSystemContext ets(mk.getSystemContext(), sett);
        LogTrace(TRACE3) << "pushControlTime() = " << ets.settings().pushControlTime();
        ets.write2db();

        // Проверка на чтение из базы
        EtsSystemContext ets2 = EtsSystemContext::readById(ets.ida());
        fail_unless(ets2.remoteAddrAirimp() == "REMADDR", "inv addr");
        fail_unless(ets2.ourAddrAirimp() == "OURADDR", "inv addr");
        fail_unless(ets2.remoteAddrEdifact() == "REMFACT", "inv addr");
        fail_unless(ets2.ourAddrEdifact() == "OURFACT", "inv addr");
        fail_unless(ets2.ourAirline() == BaseTables::Company("UT")->ida(), "inv airline");
        fail_unless(ets2.remoteAirline() == BaseTables::Company("H8")->ida(), "inv airline");
        fail_unless(ets2.systemType() == EtsSystem, "inv system type");
        fail_unless(ets2.router() == RouterId_t(1), "inv router");
        fail_unless(ets2.settings().askForControlStatus() == CouponStatus::Airport,
                    "invalid option ask_for_control_by_a");
        fail_unless(ets2.settings().sendSeparatedCos() == true,
                    "invalid option send_separated_cos");
        fail_unless(ets2.settings().xtTax() == true,
                    "invalid option xt_tax");

        LogTrace(TRACE3) << "pushControlTime() = " << ets2.settings().pushControlTime();
        fail_unless(ets2.settings().pushControlTime() == Dates::hours(72),
                    "invalid push control time");

        // read again with false option
        ets.setRemoteAirline(BaseTables::Company("U8")->ida());
        ets.setRemoteAddrEdifact("2EMFACT2");
        ets.setRemoteAddrAirimp("2EMADD2");
        ets.settings().setAskForControlByA(CouponStatus(CouponStatus::OriginalIssue));
        ets.write2db();
        ets2 = EtsSystemContext::readById(ets.ida());
        fail_unless(ets2.settings().askForControlStatus() == CouponStatus::OriginalIssue,
                    "invalid option ask_for_control_by_a");

        // Проверка на защиту от дублирования
        try
        {
            ets.write2db();
            fail("can't be here");
        }
        catch(const DuplicateRecord &e)
        {
            LogTrace(TRACE3) << e.what();
        }

        EtsSystemContext ets3(ets);
        ets3.setRemoteAirline(BaseTables::Company("U6")->ida());
        ets3.setRemoteAddrEdifact("REMFACT2");
        ets3.setRemoteAddrAirimp("REMADD2");
        ets3.write2db();

        ets.setRemoteAirline(BaseTables::Company("U6")->ida());
        try
        {
            ets.updateDb();
            /// Сменили компанию, нельзя позволить создание дублирования одинаковых АВК для ETS interline
            fail("can;t be here");
        }
        catch(DuplicateRecord &e)
        {
            LogTrace(TRACE3) << e.what();
        }
        ets3.setRemoteAirline(BaseTables::Company("U6")->ida());
        ets3.setRemoteAddrEdifact("REMFACT3");
        fail_unless(ets3.settings().sendSeparatedCos() == true,
                    "inv send_separated_cos option after insert");
        fail_unless(ets3.settings().xtTax() == true,
                    "inv xt_tax option after insert");
        ets3.settings().setSendSeparatedCos(false);
        ets3.settings().setXtTax(false);
        ets3.settings().setPushControlTime(Dates::hours(71));
        ets3.updateDb();

        ets3 = EtsSystemContext::readById(ets3.ida());
        fail_unless(ets3.settings().sendSeparatedCos() == false, "inv send_separated_cos option");
        fail_unless(ets3.settings().xtTax() == false, "inv xt_tax option");
        fail_unless(ets3.settings().pushControlTime() == Dates::hours(71), "inv pushControlTime");
    }
    catch(std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST(chk_ets_cfg_work_edifact)
{
    try
    {
        // Проверка на создание
        SystemContextMaker mk;
        mk.setRouter(RouterId_t(1));
        mk.setSysType(EtsSystem);
        mk.setOurAddrAirimp("OURADDR");
        mk.setRemoteAddrAirimp("REMADDR");
        mk.setOurAddrEdifact("1HETH");
        mk.setOurAddrEdifactExt("UT");
        mk.setRemoteAddrEdifact("1AETH");
        mk.setRemoteAddrEdifactExt("H8");
        mk.setEdifactProfileId(edifact::EdifactProfile::idByName("SIRENA-ETS"));

        mk.setOurAirline(BaseTables::Company("UT")->ida());
        mk.setRemoteAirline(BaseTables::Company("H8")->ida());
        EtsSystemContext ets(mk.getSystemContext(), EtsSystemSettings());

        fail_unless(ets.remoteAddrEdifact()=="1AETH", "inv addr");
        fail_unless(ets.ourAddrEdifact()=="1HETH", "inv addr");
        fail_unless(ets.remoteAddrEdifactExt()=="H8", "inv addr");
        fail_unless(ets.ourAddrEdifactExt()=="UT", "inv addr");

        ets.write2db();

        // Проверка на чтение из базы
        EtsSystemContext ets2 = EtsSystemContext::readById(ets.ida());
        fail_unless(ets2.remoteAddrAirimp() == "REMADDR", "inv addr");
        fail_unless(ets2.ourAddrAirimp() == "OURADDR", "inv addr");

        fail_unless(ets2.remoteAddrEdifact() == "1AETH", "inv addr");
        fail_unless(ets2.remoteAddrEdifactSnd() == "1AETH", "inv addr");
        fail_unless(ets2.remoteAddrEdifactExt() == "H8", "inv ext addr");

        fail_unless(ets2.ourAddrEdifact() == "1HETH", "inv addr");
        fail_unless(ets2.ourAddrEdifactSnd() == "1HETH", "inv addr");
        fail_unless(ets2.ourAddrEdifactExt() == "UT", "inv ext addr");
        fail_unless(ets2.ourAirline() == BaseTables::Company("UT")->ida(), "inv airline");
        fail_unless(ets2.remoteAirline() == BaseTables::Company("H8")->ida(), "inv airline");
        fail_unless(ets2.systemType() == EtsSystem, "inv system type");
        fail_unless(ets2.router() == RouterId_t(1), "inv router");

        // Проверка на защиту от дублирования
        try
        {
            ets.write2db();
            fail("can't be here");
        }
        catch(const DuplicateRecord &e)
        {
            LogTrace(TRACE3) << e.what();
        }

        try
        {
            mk.setRemoteAddrEdifact("HEHE/UU");
            fail("inv operation");
        }
        catch(const tick_soft_except &e)
        {
            LogTrace(TRACE3) << e.what();
        }

        try
        {
            mk.setOurAddrEdifact("HEHE/UU");
            fail("inv operation");
        }
        catch(const tick_soft_except &e)
        {
            LogTrace(TRACE3) << e.what();
        }

        try
        {
            mk.setRemoteAddrEdifactExt("HEHE/UU");
            fail("inv operation");
        }
        catch(const tick_soft_except &e)
        {
            LogTrace(TRACE3) << e.what();
        }

        try
        {
            mk.setOurAddrEdifactExt("HEHE/UU");
            fail("inv operation");
        }
        catch(const tick_soft_except &e)
        {
            LogTrace(TRACE3) << e.what();
        }
    }
    catch(std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST(chk_read_interline)
{
    try
    {
        init_locale(Environment::TlgHandler);
        EtsSystemContext::initEdifactAndCreateDefault("ETH8", "ETY1",
                                                     BaseTables::Company("H8")->ida(),
                                                     BaseTables::Company("Y1")->ida());

        EtsSystemContext *cont =
                EtsSystemContext::readInterlineAgreement(BaseTables::Company("Y1")->ida(),
                                                         BaseTables::Company("H8")->ida());

        fail_unless(cont != 0, "readInterlineAgreement failed");
    }
    catch(std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST(chk_ets_cfg_work_spliter)
{
    try
    {
        std::string OurAddrEdifact, OurAddrEdifactExt;
        SystemContext::splitEdifactAddrs("HELLO",
                                         OurAddrEdifact, OurAddrEdifactExt);
        fail_unless(OurAddrEdifact == "HELLO", "inv spliter");
        fail_unless(OurAddrEdifactExt == ""  , "inv spliter");

        SystemContext::splitEdifactAddrs("HELL/HY",
                                         OurAddrEdifact, OurAddrEdifactExt);
        fail_unless(OurAddrEdifact == "HELL", "inv spliter");
        fail_unless(OurAddrEdifactExt == "HY"  , "inv spliter");
    }
    catch (std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST(chk_ets_cfg_work_edifact2addr)
{
    // Проверка на создание
    SystemContextMaker mk;
    mk.setRouter(RouterId_t(1));
    mk.setSysType(EtsSystem);
    mk.setOurAddrAirimp("OURADDR");
    mk.setRemoteAddrAirimp("REMADDR");
    mk.setOurAddrEdifact("1HETH");
    mk.setOurAddrEdifactSnd("1HHET");
    mk.setOurAddrEdifactExt("UT");
    mk.setRemoteAddrEdifact("1AETH");
    mk.setRemoteAddrEdifactSnd("1AHET");
    mk.setRemoteAddrEdifactExt("H8");
    mk.setEdifactProfileId(edifact::EdifactProfile::idByName("SIRENA-ETS"));

    mk.setOurAirline(BaseTables::Company("UT")->ida());
    mk.setRemoteAirline(BaseTables::Company("H8")->ida());
    EtsSystemSettings sett;
    sett.setAskForControlStatus(CouponStatus(CouponStatus::OriginalIssue));
    EtsSystemContext ets(mk.getSystemContext(),sett);

    fail_unless(ets.ourAddrEdifact()=="1HETH", "inv addr");
    fail_unless(ets.ourAddrEdifactSnd()=="1HHET", "inv addr");
    fail_unless(ets.ourAddrEdifactExt()=="UT", "inv addr");
    fail_unless(ets.remoteAddrEdifact()=="1AETH", "inv addr");
    fail_unless(ets.remoteAddrEdifactSnd()=="1AHET", "inv addr");
    fail_unless(ets.remoteAddrEdifactExt()=="H8", "inv addr");

    ets.write2db();

    // Проверка на чтение из базы
    EtsSystemContext ets2 = EtsSystemContext::readById(ets.ida());
    fail_unless(ets2.remoteAddrAirimp() == "REMADDR", "inv addr");
    fail_unless(ets2.ourAddrAirimp() == "OURADDR", "inv addr");

    fail_unless(ets2.remoteAddrEdifact() == "1AETH", "inv addr");
    fail_unless(ets2.remoteAddrEdifactSnd() == "1AHET", "inv addr");
    fail_unless(ets2.remoteAddrEdifactExt() == "H8", "inv ext addr");

    fail_unless(ets2.ourAddrEdifact() == "1HETH", "inv addr");
    fail_unless(ets2.ourAddrEdifactSnd() == "1HHET", "inv addr");
    fail_unless(ets2.ourAddrEdifactExt() == "UT", "inv ext addr");

    fail_unless(ets2.ourAirline() == BaseTables::Company("UT")->ida(), "inv airline");
    fail_unless(ets2.remoteAirline() == BaseTables::Company("H8")->ida(), "inv airline");
    fail_unless(ets2.systemType() == EtsSystem, "inv system type");
    fail_unless(ets2.router() == RouterId_t(1), "inv router");
    fail_unless(ets2.settings().askForControlStatus() == CouponStatus::OriginalIssue, "invalid option");


    mk.setIda(ets2.ida());
    mk.setRemoteAddrEdifactSnd("1ALOH");
    mk.setOurAddrEdifactSnd("1HRUL");
    EtsSystemSettings sett2;
    sett2.setAskForControlStatus(CouponStatus(CouponStatus::Airport));
    sett2.setSendSeparatedCos(true);
    sett2.setPushControlTime(Dates::not_a_date_time);
    EtsSystemContext ets3 = EtsSystemContext(mk.getSystemContext(),sett2);
    ets3.updateDb();

    ets3 = EtsSystemContext::readById(ets3.ida());
    fail_unless(ets3.remoteAddrEdifactSnd() == "1ALOH", "inv addr");
    fail_unless(ets3.ourAddrEdifactSnd() == "1HRUL", "inv addr");
    fail_unless(ets3.settings().askForControlStatus() == CouponStatus::Airport, "invalid option");
    fail_unless(ets3.settings().sendSeparatedCos() == true, "inv send_separated_cos option value");

    LogTrace(TRACE3) << "ets3.settings().pushControlTime: " << ets3.settings().pushControlTime();

    fail_unless(ets3.settings().pushControlTime().is_special() == true, "pushControlTime is not special");
}
END_TEST;

START_TEST(chk_edi_init_by_answer)
{
    try
    {
        // создание
        SystemContextMaker mk;
        mk.setRouter(RouterId_t(1));
        mk.setSysType(EtsSystem);
        mk.setOurAddrAirimp("OURADDR");
        mk.setRemoteAddrAirimp("REMADDR");
        mk.setOurAddrEdifact("1HETH");
        mk.setOurAddrEdifactSnd("hello");
        mk.setOurAddrEdifactExt("UT");
        mk.setRemoteAddrEdifact("1AETH");
        mk.setRemoteAddrEdifactSnd("world");
        mk.setRemoteAddrEdifactExt("H8");
        mk.setEdifactProfileId(edifact::EdifactProfile::idByName("SIRENA-ETS"));

        mk.setOurAirline(BaseTables::Company("UT")->ida());
        mk.setRemoteAirline(BaseTables::Company("H8")->ida());
        EtsSystemContext(mk.getSystemContext(), EtsSystemSettings()).write2db();

        SystemContext *cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");
        fail_unless(cnt!=0,"SystemContext::initEdifactByAnswer failed");
    }
    catch (std::exception &e)
    {
        fail(e.what());
    }
}
END_TEST;

START_TEST(chk_system_settings_ets)
{
    // создание
    SystemContextMaker mk;
    mk.setRouter(RouterId_t(1));
    mk.setSysType(EtsSystem);
    mk.setOurAddrAirimp("OURADDR");
    mk.setRemoteAddrAirimp("REMADDR");
    mk.setOurAddrEdifact("1HETH");
    mk.setOurAddrEdifactSnd("hello");
    mk.setOurAddrEdifactExt("UT");
    mk.setRemoteAddrEdifact("1AETH");
    mk.setRemoteAddrEdifactSnd("world");
    mk.setRemoteAddrEdifactExt("H8");
    mk.setEdifactProfileId(edifact::EdifactProfile::idByName("SIRENA-ETS"));

    mk.setOurAirline(BaseTables::Company("UT")->ida());
    mk.setRemoteAirline(BaseTables::Company("H8")->ida());
    EtsSystemContext(mk.getSystemContext(), EtsSystemSettings()).write2db();

    SystemContext *cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");
    fail_unless(cnt!=0,"SystemContext::initEdifactByAnswer failed");

    fail_unless(cnt->commonSettings().debugInfo() == true, "inv default debug value");
    fail_unless(cnt->commonSettings().sendOpenRpi() == true, "inv default send_rpi value");
    fail_unless(cnt->commonSettings().sendCriInDisplays() == true, "inv default send_cri value");

    mk = SystemContextMaker(*cnt);
    mk.setSystemSettings(SystemSettings(false/*debug*/,true/*snd_rpi*/,false/*snd_cri*/));
    EtsSystemContext(mk.getSystemContext(), EtsSystemSettings()).updateDb();

    cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");
    fail_unless(cnt->commonSettings().debugInfo() == false, "inv debug value");
    fail_unless(cnt->commonSettings().sendOpenRpi() == true, "inv default send_rpi value");
    fail_unless(cnt->commonSettings().sendCriInDisplays() == false, "inv default send_cri value");

    mk = SystemContextMaker(*cnt);
    mk.setSystemSettings(SystemSettings(true/*debug*/,false/*snd_rpi*/,true/*send_cri*/));
    EtsSystemContext(mk.getSystemContext(), EtsSystemSettings()).updateDb();

    cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");
    fail_unless(cnt->commonSettings().debugInfo() == true, "inv debug value");
    fail_unless(cnt->commonSettings().sendOpenRpi() == false, "inv default send_rpi value");
    fail_unless(cnt->commonSettings().sendCriInDisplays() == true, "inv default send_cri value");
}
END_TEST;

START_TEST(chk_system_settings_crs)
{
    // создание
    SystemContextMaker mk;
    mk.setRouter(RouterId_t(1));
    mk.setSysType(CrsSystem);
    mk.setOurAddrAirimp("OURADDR");
    mk.setRemoteAddrAirimp("REMADDR");
    mk.setOurAddrEdifact("1HETH");
    mk.setOurAddrEdifactSnd("hello");
    mk.setOurAddrEdifactExt("UT");
    mk.setRemoteAddrEdifact("1AETH");
    mk.setRemoteAddrEdifactSnd("world");
    mk.setRemoteAddrEdifactExt("H8");
    mk.setEdifactProfileId(edifact::EdifactProfile::idByName("SIRENA-ETS"));

    mk.setOurAirline(BaseTables::Company("UT")->ida());
    mk.setRemoteAirline(BaseTables::Company("H8")->ida());
    CrsSystemContext(mk.getSystemContext(), "TTT", CrsSystemSettings(SystemSettings())).write2db();

    SystemContext *cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");
    fail_unless(cnt!=0,"SystemContext::initEdifactByAnswer failed");

    fail_unless(cnt->commonSettings().debugInfo() == true, "inv default debug value");
    fail_unless(cnt->commonSettings().sendOpenRpi() == true, "inv default send_rpi value");
    fail_unless(cnt->commonSettings().sendCriInDisplays() == true, "inv default send_cri value");

    mk = SystemContextMaker(*cnt);
    mk.setSystemSettings(SystemSettings(false/*debug*/,true/*snd_rpi*/,false/*snd_cri*/));
    CrsSystemContext(mk.getSystemContext(), "TTT", CrsSystemSettings(SystemSettings())).updateDb();

    cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");
    fail_unless(cnt->commonSettings().debugInfo() == false, "inv debug value");
    fail_unless(cnt->commonSettings().sendOpenRpi() == true, "inv default send_rpi value");
    fail_unless(cnt->commonSettings().sendCriInDisplays() == false, "inv default send_cri value");

    mk = SystemContextMaker(*cnt);
    mk.setSystemSettings(SystemSettings(true/*debug*/,false/*snd_rpi*/,true/*snd_cri*/));
    CrsSystemContext(mk.getSystemContext(), "TTT", CrsSystemSettings(SystemSettings())).updateDb();

    cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");
    fail_unless(cnt->commonSettings().debugInfo() == true, "inv debug value");
    fail_unless(cnt->commonSettings().sendOpenRpi() == false, "inv default send_rpi value");
    fail_unless(cnt->commonSettings().sendCriInDisplays() == true, "inv default send_cri value");
}
END_TEST;

START_TEST(chk_settings_edifact)
{
    // создание
    init_locale(Environment::Daemon);
    SystemContextMaker mk;
    mk.setRouter(RouterId_t(1));
    mk.setSysType(CrsSystem);
    mk.setOurAddrAirimp("OURADDR");
    mk.setRemoteAddrAirimp("REMADDR");
    mk.setOurAddrEdifact("1HETH");
    mk.setOurAddrEdifactSnd("hello");
    mk.setOurAddrEdifactExt("UT");
    mk.setRemoteAddrEdifact("1AETH");
    mk.setRemoteAddrEdifactSnd("world");
    mk.setRemoteAddrEdifactExt("H8");
    mk.setEdifactProfileId(edifact::EdifactProfile::idByName("SIRENA-ETS"));

    mk.setOurAirline(BaseTables::Company("UT")->ida());
    mk.setRemoteAirline(BaseTables::Company("H8")->ida());
    CrsSystemContext(mk.getSystemContext(), "TTT", CrsSystemSettings(SystemSettings())).write2db();

    SystemContext *cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");

    SystemSettingsEdi settEdi = cnt->settingsEdi(TlgHandling::TlgTypeEdifact::tktres);

    fail_unless(settEdi.send2Tvl() == true, "invalid send2Tvl option");


    mk.setEdifactProfileId(edifact::EdifactProfile::idByName("AMADEUS-1A"));
    mk.setIda(cnt->ida());
    CrsSystemContext(mk.getSystemContext(), "1A", CrsSystemSettings(SystemSettings())).updateDb();

    cnt = SystemContext::initEdifactByAnswer("world","H8","hello","UT");

    settEdi = cnt->settingsEdi(TlgHandling::TlgTypeEdifact::tktres);

    fail_unless(settEdi.send2Tvl() == false, "invalid send2Tvl option");

}
END_TEST;

START_TEST(chk_time_duration_logic)
{
    using namespace Dates;
    time_duration tdura_future(pos_infin);
    ptime d1 = second_clock::local_time() - time_duration(hours(100));
    ptime d2 = second_clock::local_time() - tdura_future;

    LogTrace(TRACE3) << "d1 = " << d1;
    LogTrace(TRACE3) << "d2 = " << d2;

    fail_unless(d1 > d2, "inv time_duration logic ?!");
    fail_unless(d2 == neg_infin, "inv time_duration logic ?!");
}
END_TEST;

#define SUITENAME "systems_cfg"
TCASEREGISTER( init, tear_down)
{
    ADD_TEST( chk_ets_cfg_work );
    ADD_TEST( chk_read_interline );
    ADD_TEST( chk_ets_cfg_work_edifact );
    ADD_TEST( chk_ets_cfg_work_spliter );
    ADD_TEST( chk_ets_cfg_work_edifact2addr );
    ADD_TEST( chk_edi_init_by_answer );
    ADD_TEST( chk_system_settings_ets );
    ADD_TEST( chk_system_settings_crs );
    ADD_TEST( chk_time_duration_logic );
    ADD_TEST( chk_settings_edifact );
}
TCASEFINISH;

#endif /*XP_TESTING*/
