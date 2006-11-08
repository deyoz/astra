#define NICKNAME "VLAD"
#include <test.h>

#include <daemon.h>
#include <ocilocal.h>
#include "oralib.h"
#include "jxtlib.h"
#include "astra_callbacks.h"
#include "astra_main.h"
#include "tlg/tlg.h"
#include "timer.h"

using namespace ServerFramework;

namespace ServerFramework{
inline QueryRunner AstraQueryRunner()
{
    return QueryRunner ( EdiHelpManager::sharedPtr<EdiHelpManager>(MSG_ANSW_STORE_WAIT_SIG,MSG_ANSW_ANSWER));
}
}

class AstraApplication : public ApplicationCallbacks
{
  public:
    AstraApplication()
    {
      Obrzapnik::getInstance()
              ->add("tlg_snd", main_snd_tcl)
              ->add("tlg_srv", main_srv_tcl)
              ->add("typeb_handler", main_typeb_handler_tcl)
              ->add("edi_handler", main_edi_handler_tcl)
              ->add("timer",main_timer_tcl)
              ->setApplicationCallbacks(this);
    }
    virtual int jxt_proc(const char *body, int blen, const char *head, int hlen,
                 char **res, int len)
    {
        ServerFramework::QueryRunner query_runner (ServerFramework::AstraQueryRunner());
      return jxtlib::JXTLib::Instance()->GetCallbacks()->Main(body,blen,head,hlen,res,len);
    }

    virtual void connect_db()
    {
    	ApplicationCallbacks::connect_db();
    	OraSession.Initialize(LD);
    }
/*    virtual void disconnect_db()
    {
        return disconnect_oracle();
    }
*/
    virtual void on_exit(void)
    {
    }
    virtual int tcl_init(Tcl_Interp *);

    virtual int tcl_start(Tcl_Interp *interp)
    {
        return ServerFramework::ApplicationCallbacks::
            tcl_start(interp);
    }

    virtual void levC_app_init();
/*
    virtual int nosir_proc(int argc,char **argv);
    virtual void help_nosir()
    {
        return help_nosir_user();
    }
*/
};

/*
void term3(int signo)
{
  ProgError(STDLOG,"Killed in action :-( by %d",signo);
  exit(1);
}*/

void AstraApplication::levC_app_init()
{
  if(init_locale()<0)
  {
    FlushLog();
    puts("Error retrieving site information");
    term3(SIGINT);
  }
  init_edifact();
}

int AstraApplication::tcl_init(Tcl_Interp *interp)
{
    ServerFramework::ApplicationCallbacks::tcl_init(interp);
#if 0
    if(!Tcl_CreateObjCommand(interp,"get_csa",get_csa,(ClientData)0,0)){
        fprintf(stderr,"%s\n",Tcl_GetString(Tcl_GetObjResult(interp)));
        return -1;
    }
    if(!Tcl_CreateObjCommand(interp,"check_context_size",
                tcl_check_context_size,(ClientData)0,0)){
        fprintf(stderr,"%s\n",Tcl_GetString(Tcl_GetObjResult(interp)));
        return -1;
    }
#endif /* 0 */
    AstraJxtCallbacks *ajc=new AstraJxtCallbacks();
//    AstraLocaleCallbacks *alc=new AstraLocaleCallbacks();
    return 0;
}


int main(int argc,char **argv)
{
  AstraApplication astra_app;
  return astra_app.run(argc,argv);
}

