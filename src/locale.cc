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
#include "pax_calc_data.h"

#include <jxtlib/JxtInterface.h>
#include <serverlib/ocilocal.h>
#include <serverlib/TlgLogger.h>
#include <libtlg/telegrams.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace {
class AstraTlgCallbacks: public telegrams::TlgCallbacks
{
public:
    virtual telegrams::ErrorQueue& errorQueue();
    virtual telegrams::HandlerQueue& handlerQueue();
    virtual telegrams::GatewayQueue& gatewayQueue();

    virtual void registerHandlerHook(size_t);
    virtual int getRouterInfo(int router, telegrams::RouterInfo &ri);

    virtual Expected<telegrams::TlgResult, int> putTlg2OutQueue(OUT_INFO *oi,
                                                                const char *body,
                                                                std::list<tlgnum_t>* tlgParts = 0) override;

    virtual int readTlg(const tlgnum_t& msg_id, boost::posix_time::ptime& saveTime, int &router, std::string& tlgText);
    virtual bool ttlExpired(const std::string& tlgText, bool from_our, int router, boost::optional<hth::HthInfo> hthInfo);
    virtual void readAllRouters(std::list<telegrams::RouterInfo>& routers);

};

//

#define NON_IMPLEMENTED_CALL throw EXCEPTIONS::Exception("Non-implemented call");

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

int init_locale(void)
{
    ProgTrace(TRACE1,"init_locale");
    JxtInterfaceMng::Instance();
    if(edifact::init_edifact() < 0)
        throw EXCEPTIONS::Exception("'init_edifact' error!");
    typeb_parser::typeb_template_init();
    init_tlg_callbacks();
    init_pnr_callbacks();
    init_rfisc_callbacks();
    initPassengerCallbacks();
    IAPI::init_callbacks();
    PaxCalcData::init_callbacks();
    TlgLogger::setLogging();
    return 0;
}

void init_tlg_callbacks()
{
    telegrams::Telegrams::Instance()->setCallbacks(new AstraTlgCallbacks);
}

