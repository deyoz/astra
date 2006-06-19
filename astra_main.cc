#define NICKNAME "VLAD"
#define NICKTRACE VLAD_TRACE
#include "test.h"

#include "daemon.h"
#include "jxtlib_astra.h"
#include "astra_main.h"

using namespace ServerFramework;

int xml_terminal_main(const char *body, int blen, const char *head, int hlen,
                      char **res, int len)
{
  char ans[]="hello";
  int anslen=strlen(ans);
  *res=(char *)malloc(hlen + anslen);
  memcpy(*res+hlen,ans,anslen);
  return anslen+hlen;
}

class AstraApplication : public ApplicationCallbacks
{
  public:
    AstraApplication()
    {
      Obrzapnik::getInstance()
      ->setApplicationCallbacks(this);
    }
    virtual int jxt_proc(const char *body, int blen, const char *head, int hlen,
                 char **res, int len)
    {
      return jxtlib::JXTLib::Instance()->GetCallbacks()->Main(body,blen,head,hlen,res,len);
//      return xml_terminal_main(body,blen,head,hlen,res,len);
    }
/*
    virtual void connect_db()
    {
        return connect_oracle();
    }
    virtual void disconnect_db()
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


void term3(int signo)
{
  ProgError(STDLOG,"Killed in action :-( by %d",signo);
  exit(1);
}

void AstraApplication::levC_app_init()
{
  if(init_locale()<0)
  {
    FlushLog();
    puts("Error retrieving site information");
    term3(SIGINT);
  }
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
    AstraLocaleCallbacks *alc=new AstraLocaleCallbacks();
    return 0;
}

int main(int argc,char **argv)
{
  AstraApplication astra_app;
  return astra_app.run(argc,argv);
}

