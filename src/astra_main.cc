#include <string>
#include "oralib.h"
#include "astra_callbacks.h"
#include "astra_main.h"
#include "tlg/tlg.h"
#include "timer.h"
#include "cobra.h"
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
#include "serverlib/cursctl.h"
#include "tclmon/mespro_crypt.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"
#include "exceptions.h"
#include "dev_utils.h"
#include <fstream>

int main_edi_timer_tcl(int supervisorSocket, int argc, char *argv[]);
int main_msg_handler_tcl(int supervisorSocket, int argc, char *argv[]);

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
      if(head[pbyte]&MSG_PUB_CRYPT) // признак передачи симм. ключа
        ctrl_body="ОБМЕН СИММЕТРИЧНЫМ КЛЮЧОМ";
      else if(head[pbyte]&MSG_BINARY) // признак передачи бинарных данных
        ctrl_body="ОБМЕН ДВОИЧНЫМИ ДАННЫМИ";
      else if (! (head[pbyte]&MSG_TEXT)) // передается XML (всегда в UTF-8)
      {
        string tmp=string(body,blen>2*_ML_CTRL_MES_FOR_XML_?2*_ML_CTRL_MES_FOR_XML_:blen);
        for(unsigned int i=tmp.size()-1;i>tmp.size()-5;--i)
        {
          if((unsigned char)tmp[i]>=0xC0 && (unsigned char)tmp[i]<=0xFD) // первый байт многобайтовой посл-ти
          {
            ProgTrace(TRACE1,"Попали в UTF-8 символ");
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
                ->add("aodb_handler", "logdaemon", main_aodb_handler_tcl)
                ->add("cobraserv", "logdaemon", main_tcp_cobra_tcl)
                ->add("cobra_handler", "logdaemon", main_cobra_handler_tcl)
                ->add("wb_garantserv", "logdaemon", main_tcp_wb_garant_tcl)
                ->add("wb_garant_handler", "logdaemon", main_wb_garant_handler_tcl)
                ->add("request_dup", "logdaemon", main_request_dup_tcl)
                ->add("edi_timer", "logdaemon", main_edi_timer_tcl)
                ->add("msg_handler", "logdaemon", main_msg_handler_tcl)
                ->add("apps_handler", "logairimp", main_apps_handler_tcl)
                ->add("apps_answer_emul", "logairimp", main_apps_answer_emul_tcl);
    }
    virtual int jxt_proc(const char *body, int blen, const char *head, int hlen,
                 char **res, int len)
    {
      ServerFramework::QueryRunner query_runner (ServerFramework::TextQueryRunner());
      query_runner.getEdiHelpManager().multiMsgidMode(true);
      OciCpp::mainSession().set7(); //переключение в OCI8 не идет, но на всякий случай подстрахуемся!
      int i= jxtlib::JXTLib::Instance()->GetCallbacks()->Main(body,blen,head,hlen,res,len);
      return i;
    }
    virtual void http_handle(ServerFramework::HTTP::reply& rep, const ServerFramework::HTTP::request& req)
    {
      OciCpp::mainSession().set7(); //это очень плохо что где-то в serverlib постоянно идет переключение на OCI8 !
      AstraHTTP::http_main(rep, req);
    }

    virtual std::tuple<std::vector<uint8_t>, std::vector<uint8_t>> internet_proc(const std::vector<uint8_t>& body, const char *head, size_t hlen) override
    {
      //ProgError(STDLOG, "OciCpp::mainSession()=%d", OciCpp::mainSession().mode());
      OciCpp::mainSession().set7(); //это очень плохо что где-то в serverlib постоянно идет переключение на OCI8 !
      return AstraWeb::internet_main(body,head,hlen);
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
      OciCpp::mainSession().set7();
         OraSession.Initialize(OciCpp::mainSession().getLd() );
    }
    virtual void on_exit(void)
    {
    }
    virtual int tcl_init(Tcl_Interp *interp)
    {
      ApplicationCallbacks::tcl_init(interp);
      AstraJxtCallbacks* astra_cb_ptr = new AstraJxtCallbacks();
      astra_cb_ptr->SetPostProcessXMLAnswerCallback(CheckTermResDoc);
      return 0;
    }

    virtual int tcl_start(Tcl_Interp *interp)
    {
      return ApplicationCallbacks::tcl_start(interp);
    }

    virtual void levC_app_init();

    virtual int nosir_proc(int argc,char **argv);
    virtual void help_nosir()
    {
      return help_nosir_user();
    }
    virtual size_t form_crypt_error(char* res, size_t res_len, const char* head, size_t hlen, int error)
    {
        return ::form_crypt_error(res,res_len,head,hlen,error);
    }
#ifdef USE_MESPRO
    virtual void getMesProParams(const char *head, int hlen, int *error, MPCryptParams &params)
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
}

int main(int argc,char **argv)
{
    std::cout<<"!!!!"<<std::endl;
/*
#ifdef XP_TESTING
    init_foreign_tests();
#endif*//*XP_TESTING*/

    using namespace EXCEPTIONS;
    BCBPSections bcbp;
    std::vector<std::string> bcbp_strs =
    {"M1DESMARAIS/LUC       EABC123 YULFRAAC 0834 226F001A0025 14D>5181WW6225BAC 00851234560032A0141234567890 1AC AC 1234567890123    01KYLX58Z^164GIWVC5EH7JNT684FVNJ9"
            "1W2QA4DVN5J8K4F0L0GEQ3DF5TGBN8709HKT5D3DW3GBHFCVHMY7J5T6HFR41W2QA4DVN5J8K4F0L0GE",
     "M1DESMARAIS/LUC       EABC123 YULFRAAC 0834 226F001A0025 100^164GIWVC5EH7JNT684FVNJ9"
                                          "1W2QA4DVN5J8K4F0L0GEQ3DF5TGBN8709HKT5D3DW3GBHFCVHMY7J5T6HFR41W2QA4DVN5J8K4F0L0GE",
     "M1GRANDMAIRE/MELANIE  EABC123 GVAGDGAF 0123 228C002FOO25 130>5002A0571234567890                            Y^164GIWVC5EH7JNT684FVNJ9"
                                          "1W2QA4DVN5J8K4F0L0GEQ3DF5TGBN8709HKT5D3DW3GBHFCVHMY7J5T6HFR41W2QA4DVN5J8K4F0L0GE",
     "M2DESMARAIS/LUC       EABC123 YULFRAAC 0834 226F001A0025 14D>5181WW6225BAC 00851234560032A0141234567890 1AC AC 1234567890123    20KYLX58ZDEF456 FRAGVALH 3664 227C012C0002 12E2A"
      "0140987654321 1AC AC 1234567890123    2PCNWQ^164GIWVC5EH7JNT684FVNJ91W2QA4DVN5J8K4F0L0GEQ3DF5TGBN8709HKT5D3DW3GBHFCVHMY7J5T6HFR41W2QA4DVN5J8K4F0L0GE",
     "M1GRANDMAIRE/MELANIE  EABC123 GVAGDGAF 0123 228C002FOO25 130>5002A0571234567890                         20KYDEF456 CDGDTWNW 0049 228F001A0002 12C2A012098765432101"
     "                       2PC ^164GIWVC5EH7JNT684FVNJ91W2QA4DVN5J8K4F0L0GEQ3DF5TGBN8709HKT5D3DW3GBHFCVHMY7J5T6HFR41W2QA4DVN5J8K4F0L0GE"
    };
    std::string real1 = "M1IVANOV/IVAN         E0847CN TJMVKOUT 969  327Y          49>50000298/2408011237  09TDD8PS7774441110";
    std::fstream file;
    char w[1000];
    file.open("/home/roman/barcode/test", std::ios::in);
    file.getline(w, sizeof(w));
    std::cout<<"Got raw string:"<<"\n";
    std::cout<<w<<"\n";
    //BCBPSections::test_bcbp_build();
    std::string bcbp_str = w; //= BCBPSections::test_bcbp_build();
    std::cout<<"\n"<<"Parsed data:"<<"\n\n";
    #define TRY(X) try {X} catch(Exception e) {std::cout<<"Exception: "<<e.what()<<"\n";} catch(...){std::cout<<"unknown exception"<<"\n";}



    try{
        //char id_ad(int i);
        //int date(int i);
        //int airline();
        /*boost::optional<bool> fast_track(int i);
        std::string airline_specific();
        int num_repeated_sections();*/
       using namespace BCBPSectionsEnums;
       BCBPSections::get(bcbp_str, 0,  bcbp_str.size(), bcbp, false);
            TRY(std::cout<<"passenger name: "<<bcbp.unique.passengerName().second<<"\n";)
              std::cout.flush();
            TRY(std::cout<<"passenger surname: "<<bcbp.unique.passengerName().first<<"\n";)
              std::cout.flush();
             TRY(std::cout<<"electronic ticket indicator "<<bcbp.electronic_ticket_indicator()<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"version number "<<to_string(bcbp.version())<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"passenger description: "<<to_string(bcbp.passenger_description())<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"source of checkin: "<<to_string(bcbp.source_of_checkin())<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"source of boarding pass issuance: "<<to_string(bcbp.source_of_boarding_pass_issuance())<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"date of boarding pass issuance: "<<to_string(bcbp.date_of_boarding_pass_issuance())<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"doc type: "<<to_string(bcbp.doc_type())<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"airline of boarding pass issuance: "<<bcbp.airline_of_boarding_pass_issuance()<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"baggage plate nums: "<<to_string(bcbp.baggage_plate_nums_as_str())<<"\n";)
             std::cout.flush();
             for(int i = 0; i < bcbp.repeated.size(); i++)
             {   TRY(std::cout<<"Repeated Section N "<<i<<":\n";)
                 std::cout.flush();
                 TRY(std::cout<<"operating carrier pnr code: "<<bcbp.operatingCarrierPNR(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"from city airport: "<<bcbp.from_city_airport(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"to city airport: "<<bcbp.to_city_airport(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"operating carrier designator: "<<bcbp.operating_carrier_designator(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"flight number num: "<<bcbp.flight_number(i).first<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"flight number letter: "<<bcbp.flight_number(i).second<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"date of flight: "<<std::to_string(bcbp.date_of_flight(i))<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"compartment code: "<<bcbp.compartment_code(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"seat number: "<<bcbp.seat_number(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"check in seq number: "<<bcbp.check_in_seq_number(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"passenger status: "<<bcbp.passenger_status(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"airline num code "<<to_string(bcbp.airline_num_code(i))<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"doc serial num: "<<bcbp.doc_serial_num(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"selectee "<<bcbp.selectee(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"international doc verification: "<<bcbp.international_doc_verification(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"marketing carrier designator: "<<bcbp.marketing_carrier_designator(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"frequent flyer airline designator: "<<bcbp.frequent_flyer_airline_designator(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"frequent flyer num: "<<bcbp.frequent_flyer_num(i)<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"free baggage allowance: "<<to_string(bcbp.free_baggage_allowance(i))<<"\n";)
                 std::cout.flush();
                 TRY(std::cout<<"fast track: "<<to_string(bcbp.fast_track(i))<<"\n";)
                 std::cout.flush();
             }
             TRY(std::cout<<"Security data: "<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"type of security data: "<<bcbp.type_of_security_data()<<"\n";)
             std::cout.flush();
             TRY(std::cout<<"security data: "<<bcbp.security()<<"\n";)
             std::cout.flush();

          }

    catch(Exception e)
    {   //std::cout<<bcbp_str.size()<<std::endl;
        std::cout<<"Exception: ";
        std::cout<<e.what();
    }
    catch(...)
    {
        std::cout<<"unknown exception";
    }

    /*ServerFramework::setApplicationCallbacks<AstraApplication>();
    return ServerFramework::applicationCallbacks()->run(argc,argv);*/
    return 0;
}
