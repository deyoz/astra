#include "oralib.h"
#include "tlg/tlg.h"
#include "exceptions.h"
#include "astra_main.h"
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

#define NON_IMPLEMETED_CALL throw EXCEPTIONS::Exception("Non-implemented call");

telegrams::ErrorQueue& AstraTlgCallbacks::errorQueue()
{
    NON_IMPLEMETED_CALL
}

telegrams::HandlerQueue& AstraTlgCallbacks::handlerQueue()
{
    NON_IMPLEMETED_CALL
}

telegrams::GatewayQueue& AstraTlgCallbacks::gatewayQueue()
{
    NON_IMPLEMETED_CALL
}

void AstraTlgCallbacks::registerHandlerHook(size_t)
{
    NON_IMPLEMETED_CALL
}

int AstraTlgCallbacks::getRouterInfo(int router, telegrams::RouterInfo &ri)
{
    NON_IMPLEMETED_CALL
}

Expected<telegrams::TlgResult, int> AstraTlgCallbacks::putTlg2OutQueue(OUT_INFO *oi, const char *body, std::list<tlgnum_t>* tlgParts)
{
    NON_IMPLEMETED_CALL
}

int AstraTlgCallbacks::readTlg(const tlgnum_t& msg_id, boost::posix_time::ptime& saveTime, int &router, std::string& tlgText)
{
    NON_IMPLEMETED_CALL
}

bool AstraTlgCallbacks::ttlExpired(const std::string& tlgText, bool from_our, int router, boost::optional<hth::HthInfo> hthInfo)
{
    NON_IMPLEMETED_CALL
}

void AstraTlgCallbacks::readAllRouters(std::list<telegrams::RouterInfo>& routers)
{
    NON_IMPLEMETED_CALL
}

#undef NON_IMPLEMETED_CALL

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

int init_locale(void)
{
    ProgTrace(TRACE1,"init_locale");
    JxtInterfaceMng::Instance();
    if(edifact::init_edifact() < 0)
        throw EXCEPTIONS::Exception("'init_edifact' error!");
    init_tlg_callbacks();
    TlgLogger::setLogging();
    return 0;
}

void init_tlg_callbacks()
{
    telegrams::Telegrams::Instance()->setCallbacks(new AstraTlgCallbacks);
}
