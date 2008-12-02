#define NICKNAME "VLAD"
#include <test.h>
#include <string>
#include <daemon.h>
#include <ocilocal.h>
#include "oralib.h"
#include "jxtlib.h"
#include "astra_callbacks.h"
#include "astra_main.h"
#include "tlg/tlg.h"
//#include "file_server/msg_server.h"
#include "timer.h"
#include "sirena_queue.h"
#include "xml_stuff.h"

using namespace ServerFramework;

extern "C" void help_nosir_user();
extern "C" int main_nosir_user(int argc,char **argv);

namespace ServerFramework{
inline QueryRunner AstraQueryRunner()
{
    return QueryRunner ( EdiHelpManager::sharedPtr<EdiHelpManager>(MSG_ANSW_STORE_WAIT_SIG,MSG_ANSW_ANSWER));
}
}

int astraMsgControl(int type /* 0 - request, 1 - answer */,
                     const char *head, int hlen, const char *body, int blen)
{
  using namespace std;
  string ctrl_body(body,blen);

  switch(head[0])
  {
    case 1: // Text terminal
      break;
    case 2: // Internet
    case 3: // JXT
    {
      int pbyte=getParamsByteByGrp(head[0]);
      if(head[pbyte]&MSG_PUB_CRYPT) // ¯à¨§­ ª ¯¥à¥¤ ç¨ á¨¬¬. ª«îç 
        ctrl_body="ŽŒ… ‘ˆŒŒ…’ˆ—›Œ Š‹ž—ŽŒ";
      else if(head[pbyte]&MSG_BINARY) // ¯à¨§­ ª ¯¥à¥¤ ç¨ ¡¨­ à­ëå ¤ ­­ëå
        ctrl_body="ŽŒ… „‚Žˆ—›Œˆ „€›Œˆ";
      else if (! (head[pbyte]&MSG_TEXT)) // ¯¥à¥¤ ¥âáï XML (¢á¥£¤  ¢ UTF-8)
      {
        string tmp=string(body,blen>2*_ML_CTRL_MES_FOR_XML_?2*_ML_CTRL_MES_FOR_XML_:blen);
        for(unsigned int i=tmp.size()-1;i>tmp.size()-5;--i)
        {
          if((unsigned char)tmp[i]>=0xC0 && (unsigned char)tmp[i]<=0xFD) // ¯¥à¢ë© ¡ ©â ¬­®£®¡ ©â®¢®© ¯®á«-â¨
          {
            ProgTrace(TRACE1,"®¯ «¨ ¢ UTF-8 á¨¬¢®«");
            tmp=tmp.substr(0,i);
            break;
          }
        }
        try
        {
          ctrl_body=UTF8toCP866(tmp);
        }
        catch(...)
        {
          ProgTrace(TRACE1,"UTF8toCP866() failed");
          ctrl_body=tmp;
        }
      }
      break;
    }
    case 4: // HTTP
      break;
  }
  return is_mes_control(type,head,hlen,ctrl_body.c_str(),ctrl_body.size());
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
//!!!              ->add("file_srv",main_file_srv_tcl)
              ->setApplicationCallbacks(this);
    }
    virtual int jxt_proc(const char *body, int blen, const char *head, int hlen,
                 char **res, int len)
    {
        ServerFramework::QueryRunner query_runner (ServerFramework::AstraQueryRunner());
      return jxtlib::JXTLib::Instance()->GetCallbacks()->Main(body,blen,head,hlen,res,len);
    }
    virtual int message_control(int type /* 0 - request, 1 - answer */,
                                const char *head, int hlen,
                                const char *body, int blen)
    {
      return astraMsgControl(type,head,hlen,body,blen);
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

    virtual int nosir_proc(int argc,char **argv);
    virtual void help_nosir()
    {
        return help_nosir_user();
    }

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

int AstraApplication::nosir_proc(int argc, char ** argv)
{
//     PerfomInit();

//     InitLogTime(NULL);

    Oci7Init(get_connect_string(),1);

    int res = main_nosir_user(argc,argv);
    if(res != 0) {
        make_curs("rollback").exec();
    } else {
        make_curs("commit").exec();
    }

    return res;
}


int main(int argc,char **argv)
{
  AstraApplication astra_app;
  return astra_app.run(argc,argv);
}
