#include "oralib.h"
#include <jxtlib/jxtlib.h>
#include "exceptions.h"
#include "astra_main.h"
#include "tlg/tlg.h"
#include "tlg/typeb_template_init.h"

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

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

int init_locale(void)
{
//  char  c_in[4],stracs[2],strtz[2],our_name[10];
//  if(get_param("CENTER_CITY", c_in,sizeof(c_in)-1)<0)
//  {
//    ProgError(STDLOG,"CENTER_CITY not defined");
//    return -1;
//  }
//  if(get_param("CENTER_NAME", Environ.c_name,sizeof(Environ.c_name)-1)<0)
//    strcpy(Environ.c_name,c_in);

//  strcpy(stracs,"0");
//  if(get_param("USE_ACCESS", stracs,sizeof(stracs)-1)>=0 &&
//     (*stracs!='0' && *stracs!='1'))
//  {
//    ProgError(STDLOG,"USE_ACCESS is invalid");
//    return -1;
//  }
//  Environ.access=atoi(stracs);

//  strcpy(strtz,"0");
//  if(get_option("USE_TZ", strtz,sizeof(strtz)-1)>=0 &&
//     (*strtz!='0' && *strtz!='1'))
//  {
//    ProgError(STDLOG,"USE_TZ is invalid");
//    return -1;
//  }
//  Environ.use_tz=atoi(strtz);

//  strcpy(our_name,"MOW1H");
//  get_option("OURNAME", our_name,sizeof(our_name)-1);
//  ProgTrace(TRACE3,"OurName=<%s>",our_name);

//#ifdef AIRIMP
//  if(SetEnviron()<0)
//    ProgError(STDLOG,"SetEnviron failed");
//#endif /* AIRIMP */

//  if(Oparse(CU,"begin "
//    "locale.init(:ci,:cr,:cl,:off,:c_in,:c_name,:dis,:inv,:our,:p1,:p2,:p3);"
//    " end;")
//      || Obndrs(CU,":c_in",c_in)
//      || Obndrs(CU,":c_name",Environ.c_name) ||
//      Obndri(CU,":ci",Environ.city_i) ||
//      Obndrs(CU,":cr",Environ.city_r) ||
//      Obndrs(CU,":cl",Environ.city_l) ||
//      Obndri(CU,":off",Environ.mainoff_i) ||
//      Obndri(CU,":p1",Environ.Ptc[0]) ||
//      Obndri(CU,":p2",Environ.Ptc[1]) ||
//      Obndri(CU,":p3",Environ.Ptc[2]) ||
//      Obndri(CU,":dis",Environ.distr) ||
//      Obndri(CU,":inv",Environ.invent) ||
//      Obndrs(CU,":our",our_name) ||
//      Oexec(CU))
//  {
//    oci_error(CU);
//    return -1;
//  }
//  ProgTrace(TRACE4,"%d",Environ.city_i);
//  if(init_code_list()<0)
//  {
//    ProgError(STDLOG,"init_code_list");
//    return -1;
//  }

//  if(getenv("SIRENA_INSTANCE"))
//    single_run(getenv("SIRENA_INSTANCE"));

//  jxtlib::JXTLib::Instance()->SetCallbacks(new AstraCallbacks());

    ProgTrace(TRACE1,"init_locale");
    JxtInterfaceMng::Instance();
    if(edifact::init_edifact() < 0)
        throw EXCEPTIONS::Exception("'init_edifact' error!");
    typeb_parser::typeb_template_init();
    init_tlg_callbacks();
    TlgLogger::setLogging();
    return 0;
}

void init_tlg_callbacks()
{
    telegrams::Telegrams::Instance()->setCallbacks(new AstraTlgCallbacks);
}
