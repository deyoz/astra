#include <string>
#include <memory>
#include "oralib.h"
#include "astra_callbacks.h"
#include "astra_main.h"
#include "tlg/tlg.h"
#include "timer.h"
#include "aodb.h"
#include "request_dup.h"
#include "crypt.h"
#include "web_main.h"
#include "http_main.h"
#include "obrnosir.h"
#include "config.h"

#include "jxtlib/jxtlib.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/msg_const.h"
#include "serverlib/monitor.h"
#include "serverlib/sirena_queue.h"
#ifdef ENABLE_ORACLE
#include "serverlib/cursctl.h"
#endif //ENABLE_ORACLE
#include "tclmon/mespro_crypt.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

int main_edi_timer_tcl(int supervisorSocket, int argc, char *argv[]);
int main_bag_msg_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_exchange_handler_tcl(int supervisorSocket, int argc, char *argv[]);
int main_flight_tasks_tcl(int supervisorSocket, int argc, char *argv[]);
int main_exch_checkin_result_queue_tcl(int supervisorSocket, int argc, char *argv[]);
int main_libra_log_events_daemon_tcl(int supervisorSocket, int argc, char *argv[]);

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
      if(head[pbyte]&MSG_PUB_CRYPT) // �ਧ��� ��।�� ᨬ�. ����
        ctrl_body="����� ������������ ������";
      else if(head[pbyte]&MSG_BINARY) // �ਧ��� ��।�� ������� ������
        ctrl_body="����� ��������� �������";
      else if (! (head[pbyte]&MSG_TEXT)) // ��।����� XML (�ᥣ�� � UTF-8)
      {
        string tmp=string(body,blen>2*_ML_CTRL_MES_FOR_XML_?2*_ML_CTRL_MES_FOR_XML_:blen);
        for(unsigned int i=tmp.size()-1;i>tmp.size()-5;--i)
        {
          if((unsigned char)tmp[i]>=0xC0 && (unsigned char)tmp[i]<=0xFD) // ���� ���� ��������⮢�� ���-�
          {
            ProgTrace(TRACE1,"������ � UTF-8 ᨬ���");
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
  return monitorControl::is_mes_control(type,head,hlen,ctrl_body.c_str(),ctrl_body.size());
}

class AstraApplication : public ServerFramework::ApplicationCallbacks
{
  public:
    AstraApplication()
    {
        ServerFramework::Obrzapnik::getInstance()
                ->add("timer", "logdaemon", main_timer_tcl)
                ->add("tlg_http_snd", "logairimp", main_http_snd_tcl)
                ->add("tlg_snd", "logairimp", main_snd_tcl)
                ->add("tlg_srv", "logairimp", main_srv_tcl)
                ->add("typeb_handler", "logairimp", main_typeb_handler_tcl)
                ->add("typeb_parser", "logairimp", main_typeb_parser_tcl)
                ->add("edi_handler", "logairimp", main_edi_handler_tcl)
                ->add("iapi_handler", "logairimp", main_iapi_edi_handler_tcl)
                ->add("itci_req_handler", "logairimp", main_itci_req_handler_tcl)
                ->add("itci_res_handler", "logairimp", main_itci_res_handler_tcl)
                ->add("aodb_handler", "logdaemon", main_aodb_handler_tcl)
                ->add("request_dup", "logdaemon", main_request_dup_tcl)
                ->add("edi_timer", "logdaemon", main_edi_timer_tcl)
                ->add("apps_handler", "logairimp", main_apps_handler_tcl)
                ->add("apps_answer_emul", "logairimp", main_apps_answer_emul_tcl)
                ->add("bag_msg_handler", "logdaemon", main_bag_msg_handler_tcl)
                ->add("flight_tasks", "logdaemon", main_flight_tasks_tcl)
                ->add("exch_checkin_result", "logdaemon", main_exch_checkin_result_queue_tcl)
                ->add("libra_log_events_handler", "logdaemon", main_libra_log_events_daemon_tcl)
        ;
    }
    virtual int jxt_proc(const char *body, int blen, const char *head, int hlen,
                 char **res, int len) override
    {
      ServerFramework::QueryRunner query_runner (ServerFramework::TextQueryRunner());
      query_runner.getEdiHelpManager().multiMsgidMode(true);
#ifdef ENABLE_ORACLE
      OciCpp::mainSession().set7(); //��४��祭�� � OCI8 �� ����, �� �� ��直� ��砩 �������㥬��!
#endif //ENABLE_ORACLE
      int i= jxtlib::JXTLib::Instance()->GetCallbacks()->Main(body,blen,head,hlen,res,len);
      return i;
    }
    virtual void http_handle(ServerFramework::HTTP::reply& rep, const ServerFramework::HTTP::request& req) override
    {
#ifdef ENABLE_ORACLE
      OciCpp::mainSession().set7(); //�� �祭� ���� �� ���-� � serverlib ����ﭭ� ���� ��४��祭�� �� OCI8 !
#endif // ENABLE_ORACLE
      AstraHTTP::http_main(rep, req);
    }

    virtual std::tuple<Grp2Head, std::vector<uint8_t>> internet_proc(const Grp2Head& head, const std::vector<uint8_t>& body) override
    {
      //ProgError(STDLOG, "OciCpp::mainSession()=%d", OciCpp::mainSession().mode());
#ifdef ENABLE_ORACLE
      OciCpp::mainSession().set7(); //�� �祭� ���� �� ���-� � serverlib ����ﭭ� ���� ��४��祭�� �� OCI8 !
#endif // ENABLE_ORACLE

      std::string shead(head.begin(), head.end());
      std::vector<uint8_t> h, abody;
      std::tie(h,abody) = AstraWeb::internet_main(body, shead.data(), shead.size());

      ApplicationCallbacks::Grp2Head ahead;
      std::copy_n(h.begin(), ahead.size(), ahead.begin());
      return std::make_tuple(ahead, abody);
    }

    virtual int message_control(int type /* 0 - request, 1 - answer */,
                                const char *head, int hlen,
                                const char *body, int blen) override
    {
      return astraMsgControl(type,head,hlen,body,blen);
    }
    virtual void connect_db() override
    {
         ApplicationCallbacks::connect_db();
#ifdef ENABLE_ORACLE
         OciCpp::mainSession().set7();
         OraSession.Initialize(OciCpp::mainSession().getLd() );
#endif // ENABLE_ORACLE
    }
    virtual void on_exit(void) override
    {
    }
    virtual int tcl_init(Tcl_Interp *interp) override
    {
      ApplicationCallbacks::tcl_init(interp);
      jxtlib::JXTLib::Instance()->SetCallbacks(std::make_unique<AstraJxtCallbacks>());
      return 0;
    }

    virtual int tcl_start(Tcl_Interp *interp) override
    {
      return ApplicationCallbacks::tcl_start(interp);
    }

    virtual void levC_app_init() override;

    virtual int nosir_proc(int argc,char **argv) override;
    virtual void help_nosir() override
    {
      return help_nosir_user();
    }
    virtual size_t form_crypt_error(char* res, size_t res_len, const char* head, size_t hlen, int error) override
    {
        return ::form_crypt_error(res,res_len,head,hlen,error);
    }
#if defined USE_MESPRO && defined ENABLE_ORACLE
    virtual void getMesProParams(const char *head, int hlen, int *error, MPCryptParams &params) override
    {
      OciCpp::mainSession().set7();
      return ::getMesProParams(head,hlen,error,params);
    }
#endif // USE_MESPRO


};

void AstraApplication::levC_app_init()
{
  init_locale();
  ServerFramework::getQueryRunner().getEdiHelpManager().multiMsgidMode(true);
}

int AstraApplication::nosir_proc(int argc, char ** argv)
{
  return main_nosir_user(argc,argv);
}

static void init_foreign_tests()
{
    void init_edi_msg_tests();
    init_edi_msg_tests();

    void initTimaticTests();
    initTimaticTests();
}

int main(int argc,char **argv)
{
#ifdef XP_TESTING
    init_foreign_tests();
#endif/*XP_TESTING*/
    ServerFramework::setApplicationCallbacks<AstraApplication>();
    return ServerFramework::applicationCallbacks()->run(argc,argv);
}
