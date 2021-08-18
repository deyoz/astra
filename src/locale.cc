#include "oralib.h"
#include "exceptions.h"
#include "astra_main.h"
#include "astra_pnr.h"
#include "etick.h"
#include "custom_alarms.h"
#include "iapi_interaction.h"
#include "tlg/tlg.h"
#include "tlg/typeb_template_init.h"
#include "passenger_callbacks.h"
#include "base_callbacks.h"
#include "apps_interaction.h"
#include "dbostructures.h"
#include "pax_calc_data.h"
#include "pg_session.h"
#include "PgOraConfig.h"
#include "dbostructures.h"

#include <jxtlib/JxtInterface.h>
#include <jxtlib/jxtlib_dbpg_callbacks.h>
#ifdef ENABLE_ORACLE
#include <jxtlib/jxtlib_dbora_callbacks.h>
#endif //ENABLE_ORACLE
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/TlgLogger.h>
#include <serverlib/EdiHelpDbPgCallbacks.h>
#include <libtlg/telegrams.h>
#include <edilib/edilib_dbora_callbacks.h>
#include <edilib/edilib_dbpg_callbacks.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

#define NON_IMPLEMENTED_CALL throw EXCEPTIONS::Exception("Non-implemented call");

namespace {
class AstraTlgCallbacks: public telegrams::TlgCallbacks
{
public:
    virtual telegrams::ErrorQueue& errorQueue() override;
    virtual telegrams::HandlerQueue& handlerQueue() override;
    virtual telegrams::GatewayQueue& gatewayQueue() override;

    virtual void registerHandlerHook(size_t) override;
    virtual int getRouterInfo(int router, telegrams::RouterInfo &ri) override;

    virtual Expected<telegrams::TlgResult, int> putTlg2OutQueue(OUT_INFO *oi,
                                                                const char *body,
                                                                std::list<tlgnum_t>* tlgParts = 0) override;

    virtual int readTlg(const tlgnum_t& msg_id, boost::posix_time::ptime& saveTime, int &router, std::string& tlgText) override;
    virtual bool ttlExpired(const std::string& tlgText, bool from_our, int router, boost::optional<hth::HthInfo> hthInfo) override;
    virtual void readAllRouters(std::list<telegrams::RouterInfo>& routers) override;

    virtual bool tlgIsHth(const tlgnum_t&) override;
    virtual int readHthInfo(const tlgnum_t&, hth::HthInfo& hthInfo) override;
    virtual int writeHthInfo(const tlgnum_t&, const hth::HthInfo& hthInfo) override;
    virtual void deleteHth(const tlgnum_t&) override;

    virtual boost::optional<tlgnum_t> nextNum() override {
        NON_IMPLEMENTED_CALL
    }
    virtual tlgnum_t nextExpressNum() override {
        NON_IMPLEMENTED_CALL
    }
    virtual void splitError(const tlgnum_t &num) override {
        NON_IMPLEMENTED_CALL
    }
    virtual bool saveBadTlg(const telegrams::AIRSRV_MSG &tlg, int error) override
    {
        NON_IMPLEMENTED_CALL
    }
    virtual Expected<telegrams::TlgResult, int> putTlg(const std::string &tlgText,
                                                       telegrams::tlg_text_filter filter = telegrams::tlg_text_filter(),
                                                       int from_addr = 0, int to_addr = 0, hth::HthInfo *hth = 0) override
    {
        NON_IMPLEMENTED_CALL
    }
    virtual void savepoint(const std::string &sp_name) const override {
        NON_IMPLEMENTED_CALL
    }
    virtual void rollback(const std::string &sp_name) const override {
        NON_IMPLEMENTED_CALL
    }
    virtual void rollback() const override {
        NON_IMPLEMENTED_CALL
    }
};

//

telegrams::ErrorQueue& AstraTlgCallbacks::errorQueue()
{
    NON_IMPLEMENTED_CALL
}

telegrams::HandlerQueue& AstraTlgCallbacks::handlerQueue()
{
    NON_IMPLEMENTED_CALL
}

telegrams::GatewayQueue& AstraTlgCallbacks::gatewayQueue()
{
    NON_IMPLEMENTED_CALL
}

void AstraTlgCallbacks::registerHandlerHook(size_t)
{
    NON_IMPLEMENTED_CALL
}

int AstraTlgCallbacks::getRouterInfo(int router, telegrams::RouterInfo &ri)
{
    NON_IMPLEMENTED_CALL
}

Expected<telegrams::TlgResult, int> AstraTlgCallbacks::putTlg2OutQueue(OUT_INFO *oi, const char *body, std::list<tlgnum_t>* tlgParts)
{
    NON_IMPLEMENTED_CALL
}

int AstraTlgCallbacks::readTlg(const tlgnum_t& msg_id, boost::posix_time::ptime& saveTime, int &router, std::string& tlgText)
{
    NON_IMPLEMENTED_CALL
}

bool AstraTlgCallbacks::ttlExpired(const std::string& tlgText, bool from_our, int router, boost::optional<hth::HthInfo> hthInfo)
{
    NON_IMPLEMENTED_CALL
}

void AstraTlgCallbacks::readAllRouters(std::list<telegrams::RouterInfo>& routers)
{
    NON_IMPLEMENTED_CALL
}

bool AstraTlgCallbacks::tlgIsHth(const tlgnum_t& msgId)
{
    DbCpp::CursCtl cur = make_db_curs(
"select 1 from TEXT_TLG_H2H where MSG_ID = :id",
                PgOra::getROSession("TEXT_TLG_H2H"));
    cur
        .stb()
        .bind(":id", msgId.num.get())
        .EXfet();
    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        return false;
    }

    return true;
}

int AstraTlgCallbacks::readHthInfo(const tlgnum_t& msgId, hth::HthInfo& hthInfo)
{
    std::string type, qri5, qri6, remAddrNum, part;
    int end = 0;
    DbCpp::CursCtl cur = make_db_curs(
"select TYPE, SNDR, RCVR, TPR, ERR, PART, \"END\", QRI5, QRI6, REM_ADDR_NUM "
"from TEXT_TLG_H2H where MSG_ID = :id",
                PgOra::getROSession("TEXT_TLG_H2H"));
    cur
        .stb()
        .autoNull()
        .bind(":id", msgId.num.get())
        .def(type)
        .def(hthInfo.sender)
        .def(hthInfo.receiver)
        .def(hthInfo.tpr)
        .def(hthInfo.why)
        .defNull(part, std::string())
        .defNull(end, 0)
        .def(qri5)
        .def(qri6)
        .def(remAddrNum)
        .EXfet();
    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        LogTrace(TRACE5) << "Can't get header param of HTH tlg NO_DATA_FOUND for msg_id: " <<  msgId;
        return -1;
    }
    ASSERT(part.length() == 1 || part.length() == 0);
    hthInfo.type = type[0];
    hthInfo.part = part.empty() ? 0 : part[0];
    hthInfo.end  = end;
    hthInfo.qri5 = qri5[0];
    hthInfo.qri6 = qri6[0];
    hthInfo.remAddrNum = remAddrNum[0];

    hth::trace(TRACE5, hthInfo);

    return 0;
}

int AstraTlgCallbacks::writeHthInfo(const tlgnum_t& msgId, const hth::HthInfo& hthInfo)
{
    LogTrace(TRACE5) << hthInfo;
    try {
        Dates::ptime curr_tm = Dates::currentDateTime();
        make_db_curs(
"insert into TEXT_TLG_H2H (MSG_ID, TYPE, RCVR, SNDR, TPR, QRI5, QRI6,"
" PART, \"END\", REM_ADDR_NUM, ERR, TIMESTAMP) "
"values (:msg_id, :hth_type, :hth_rcvr, :hth_sndr, :hth_tpr, :hth_qri5, :hth_qri6,"
" :hth_part, :hth_end, :rem_addr_num, :hth_err, :curr_tm)",
                    PgOra::getRWSession("TEXT_TLG_H2H"))
                .stb()
                .bind(":msg_id", msgId.num.get())
                .bind(":hth_type", std::string(1, hthInfo.type))
                .bind(":hth_rcvr", hthInfo.receiver).bind(":hth_sndr", hthInfo.sender)
                .bind(":hth_tpr", hthInfo.tpr)
                .bind(":hth_part", std::string(1, hthInfo.part))
                .bind(":hth_end", (int)hthInfo.end)
                .bind(":hth_qri5", std::string(1, hthInfo.qri5))
                .bind(":hth_qri6", std::string(1, hthInfo.qri6))
                .bind(":rem_addr_num", std::string(1, hthInfo.remAddrNum))
                .bind(":hth_err", hthInfo.why)
                .bind(":curr_tm", curr_tm)
                .exec();
    } catch (const comtech::Exception& e) {
        LogTrace(TRACE0) << hthInfo;
        LogError(STDLOG) << "writeHthInfo failed " << msgId << " : " << e.what();
        return 1;
    }
    return 0;
}

void AstraTlgCallbacks::deleteHth(const tlgnum_t& msgId)
{
    make_db_curs(
        "delete from TEXT_TLG_H2H where MSG_ID = :msg_id",
        PgOra::getRWSession("TEXT_TLG_H2H"))
        .stb()
        .bind(":msg_id", msgId.num.get())
        .exec();
}

#undef NON_IMPLEMENTED_CALL

//---------------------------------------------------------------------------------------

class EtickPnrCallbacks: public Ticketing::AstraPnrCallbacks
{
public:
    virtual void afterReceiveAirportControl(const Ticketing::WcCoupon& cpn)
    {
        ETStatusInterface::AfterReceiveAirportControl(cpn);
    }
    virtual void afterReturnAirportControl(const Ticketing::WcCoupon& cpn)
    {
        ETStatusInterface::AfterReturnAirportControl(cpn);
    }
};

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

void init_pnr_callbacks()
{
    CallbacksSingleton<Ticketing::AstraPnrCallbacks>::Instance()->setCallbacks(new EtickPnrCallbacks);
}

void init_edilib_callbacks()
{
    if(PgOra::supportsPg("EDISESSION")) {
        edilib::EdilibDbCallbacks::setEdilibDbCallbacks(new edilib::EdilibPgCallbacks(PgCpp::getPgManaged()));
    } else {
#ifdef ENABLE_ORACLE
        edilib::EdilibDbCallbacks::setEdilibDbCallbacks(new edilib::EdilibOraCallbacks());
#endif //ENABLE_ORACLE
    }
}

void init_edihelp_callbacks()
{
    ServerFramework::EdiHelpDbCallbacks::setEdiHelpDbCallbacks(new ServerFramework::EdiHelpDbPgCallbacks(PgCpp::getPgManaged()));
}

void init_jxtlib_callbacks()
{
    if (PgOra::supportsPg("CONT"))
        jxtlib::JxtlibDbCallbacks::setJxtlibDbCallbacks(new jxtlib::JxtlibDbPgCallbacks(PgCpp::getPgManaged()));
#ifdef ENABLE_ORACLE
    else
        //default initialization in jxtlib with oracle callbacks
        jxtlib::JxtlibDbCallbacks::setJxtlibDbCallbacks(new jxtlib::JxtlibDbOraCallbacks());
#endif //ENABLE_ORACLE
}

int init_locale(void)
{
    ProgTrace(TRACE1,"init_locale");
    JxtInterfaceMng::Instance();
    init_edilib_callbacks();
    if(edifact::init_edifact() < 0)
        throw EXCEPTIONS::Exception("'init_edifact' error!");
    typeb_parser::typeb_template_init();
    init_tlg_callbacks();
    init_edihelp_callbacks();
    init_jxtlib_callbacks();
    init_pnr_callbacks();
    init_rfisc_callbacks();
    initPassengerCallbacks();
    dbo::initStructures();
    APPS::init_callbacks();
    IAPI::init_callbacks();
    PaxCalcData::init_callbacks();
    TlgLogger::setLogging();
    return 0;
}

void init_locale_throwIfFailed()
{
    if(init_locale() < 0) {
        throw EXCEPTIONS::Exception("init_locale failed");
    }
}

void init_tlg_callbacks()
{
    telegrams::Telegrams::Instance()->setCallbacks(new AstraTlgCallbacks);
}

