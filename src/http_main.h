#ifndef __HTTP_MAIN_H__
#define __HTTP_MAIN_H__

#include <string>
#include <map>
#include "serverlib/http_parser.h"
#include "web_main.h"
#include "cuws_main.h"

namespace AstraHTTP
{

#define SPP_SYNCH_JXT_INTERFACE_ID "SppSynch"
#define CHECKIN_JXT_INTERFACE_ID "CheckIn"
#define TELEGRAM_JXT_INTERFACE_ID "Telegram"
#define KIOSK_REQUEST_JXT_INTERFACE_ID "KioskRequest"
#define KIOSK_CONFIG_JXT_INTERFACE_ID "KioskConfig"
#define STAT_JXT_INTERFACE_ID "stat"
#define HTML_JXT_INTERFACE_ID "html"
#define SVC_SIRENA_JXT_INTERFACE_ID "SvcSirena"
#define PRINT_JXT_INTERFACE_ID "print"
#define ZAMAR_PAXCTL_JXT_INTERFACE_ID "ZamarPaxCtl"
#define ZAMAR_SBDO_JXT_INTERFACE_ID "ZamarSBDO"
#define MOBILE_PAYMENT_JXT_INTERFACE_ID "MobilePayment"

struct HTTPClient;

void CrewPostProcessXMLAnswer();
void TlgPostProcessXMLAnswer();
void ZamarPostProcessXMLAnswer();

struct JxtInfo {
  std::string interface;

  typedef void (*TPostProc)();

  TPostProc post_proc;
  JxtInfo(const std::string& name, TPostProc proc):
    interface(name), post_proc(proc) {}
  JxtInfo() : post_proc(NULL) {}
};

struct ci_less
{
  // case-independent (ci) compare_less binary function
  struct nocase_compare
  {
    bool operator() (const unsigned char& c1, const unsigned char& c2) const {
        return tolower (c1) < tolower (c2);
    }
  };
  bool operator() (const std::string & s1, const std::string & s2) const {
    return std::lexicographical_compare
      (s1.begin (), s1.end (),   // source range
      s2.begin (), s2.end (),   // dest range
      nocase_compare ());  // comparison
  }
};

struct httpParams:public std::map<std::string,std::string,ci_less>
{
  void operator<<(const ServerFramework::HTTP::request::Headers &hdrs ) {
     for ( auto header : hdrs  ) {
       insert( make_pair( header.name, header.value ) );
     }
  }
  std::string trace( ) {
    std::ostringstream buf;
    buf << "heardes: " << std::endl;
    for ( auto header : *this  ) {
      buf << "'" << header.first << "'=" << header.second << "'";
    }
    return buf.str();
  }
};

namespace EXCHANGE_TYPE {
    static const std::string CREWCHECKIN    = "CREWCHECKIN";
    static const std::string KIOSK_SERVER   = "KIOSK_SERVER";
    static const std::string KIOSK          = "KIOSK";
    static const std::string KUFSTAT        = "KUFSTAT";
    static const std::string SBDO           = "SBDO";
    static const std::string PAX_CTL        = "PAX_CTL";
    static const std::string SINHRONSVO     = "SINHRONSVO";
    static const std::string SPPUFA         = "SPPUFA";
    static const std::string STSTAT         = "STSTAT";
    static const std::string TLG_SRV        = "TLG_SRV";
    static const std::string PIECE_CONCEPT  = "PIECE_CONCEPT";
    static const std::string MOBILE_PAYMENT = "MOBILE_PAYMENT";
    static const std::string HTML           = "HTML";
    static const std::string CUWS           = "CUWS";
}

struct HTTPClient
{
    private:
        typedef std::map<std::string, JxtInfo> TOperationMap;
        typedef std::map<std::string, TOperationMap> TExchangeTypeMap;

        const TExchangeTypeMap jxt_interface {
            {EXCHANGE_TYPE::CUWS,
                {
                    {"CUWS", JxtInfo(CUWS_JXT_IFACE_ID, nullptr)}
                }
            },{EXCHANGE_TYPE::CREWCHECKIN,
                {
                    {"CREWCHECKIN", JxtInfo(CHECKIN_JXT_INTERFACE_ID,   CrewPostProcessXMLAnswer)}
                }
            },{EXCHANGE_TYPE::KIOSK_SERVER,
                {
                    {"EventToServer", JxtInfo(KIOSK_REQUEST_JXT_INTERFACE_ID, NULL)},
                    {"PingKiosk",     JxtInfo(KIOSK_REQUEST_JXT_INTERFACE_ID, NULL)}
                }
            },{EXCHANGE_TYPE::KIOSK,
                {
                    {"ViewCraftKiosk",  JxtInfo(KIOSK_REQUEST_JXT_INTERFACE_ID, NULL)},
                    {"AppParamsKiosk",  JxtInfo(KIOSK_CONFIG_JXT_INTERFACE_ID,  NULL)},
                    {"AppAliasesKiosk", JxtInfo(KIOSK_CONFIG_JXT_INTERFACE_ID,  NULL)}
                }
            },{EXCHANGE_TYPE::KUFSTAT,
                {
                    {"get_resource",  JxtInfo(HTML_JXT_INTERFACE_ID,      NULL)},
                    {"kuf_file",      JxtInfo(TELEGRAM_JXT_INTERFACE_ID,  NULL)},
                    {"kuf_stat_flts", JxtInfo(TELEGRAM_JXT_INTERFACE_ID,  NULL)},
                    {"kuf_stat",      JxtInfo(TELEGRAM_JXT_INTERFACE_ID,  NULL)}
                }
            },{EXCHANGE_TYPE::SBDO,
                {
                    {"PassengerSearchSBDO",           JxtInfo(ZAMAR_SBDO_JXT_INTERFACE_ID,     ZamarPostProcessXMLAnswer)},
                    {"PassengerBaggageTagAdd",        JxtInfo(ZAMAR_SBDO_JXT_INTERFACE_ID,     ZamarPostProcessXMLAnswer)},
                    {"PassengerBaggageTagConfirm",    JxtInfo(ZAMAR_SBDO_JXT_INTERFACE_ID,     ZamarPostProcessXMLAnswer)},
                    {"PassengerBaggageTagRevoke",     JxtInfo(ZAMAR_SBDO_JXT_INTERFACE_ID,     ZamarPostProcessXMLAnswer)}
                }
            },{EXCHANGE_TYPE::PAX_CTL,
                {
                    {"PassengerSearchPaxCtl",         JxtInfo(ZAMAR_PAXCTL_JXT_INTERFACE_ID,   ZamarPostProcessXMLAnswer)}
                }
            },{EXCHANGE_TYPE::SINHRONSVO,
                {
                    {"SaveSinhronSPP", JxtInfo(SPP_SYNCH_JXT_INTERFACE_ID, NULL)}
                }
            },{EXCHANGE_TYPE::PIECE_CONCEPT,
                {
                    {"piece_concept", JxtInfo(SVC_SIRENA_JXT_INTERFACE_ID,    NULL)}
                }
            },{EXCHANGE_TYPE::SPPUFA,
                {
                    {"SaveUFASPP", JxtInfo(SPP_SYNCH_JXT_INTERFACE_ID, NULL)}
                }
            },{EXCHANGE_TYPE::STSTAT,
                {
                    {"stat_srv", JxtInfo(STAT_JXT_INTERFACE_ID, NULL)}
                }
            },{EXCHANGE_TYPE::TLG_SRV,
                {
                    {"tlg_srv", JxtInfo(TELEGRAM_JXT_INTERFACE_ID,  TlgPostProcessXMLAnswer)},
                    {"kick",    JxtInfo(TELEGRAM_JXT_INTERFACE_ID,  TlgPostProcessXMLAnswer)}
                }
            },{EXCHANGE_TYPE::MOBILE_PAYMENT,
                {
                    {"search_passengers",   JxtInfo(MOBILE_PAYMENT_JXT_INTERFACE_ID, nullptr)},
                    {"search_flights",      JxtInfo(MOBILE_PAYMENT_JXT_INTERFACE_ID, nullptr)},
                    {"get_client_perms",    JxtInfo(MOBILE_PAYMENT_JXT_INTERFACE_ID, nullptr)},
                    {"get_passenger_info",  JxtInfo(MOBILE_PAYMENT_JXT_INTERFACE_ID, nullptr)}
                }
            },{EXCHANGE_TYPE::HTML,
                {
                    {"GetBPTags", JxtInfo(WEB_JXT_IFACE_ID,                 NULL)},
                    // Запрос с одинаковым названием, но в разных классах
                    // данная мэпа не подходит для такого случая
                    // т.к. затирается обработчик
                    // {"GetPrintDataBP", JxtInfo(PRINT_JXT_INTERFACE_ID,      NULL)},
                    {"GetPrintDataBP", JxtInfo(WEB_JXT_IFACE_ID,            NULL)},
                    {"GetGRPPrintDataBP", JxtInfo(PRINT_JXT_INTERFACE_ID,   NULL)},
                    {"GetGRPPrintData", JxtInfo(PRINT_JXT_INTERFACE_ID,     NULL)},
                    {"GetImg", JxtInfo(PRINT_JXT_INTERFACE_ID,              NULL)},
                    {"GetPrintData", JxtInfo(PRINT_JXT_INTERFACE_ID,        NULL)},
                    {"get_resource", JxtInfo(HTML_JXT_INTERFACE_ID,         NULL)},
                    {"print_bp", JxtInfo(PRINT_JXT_INTERFACE_ID,            NULL)},
                    {"print_bp2", JxtInfo(PRINT_JXT_INTERFACE_ID,           NULL)}
                    // Не используются ?
                    // {"SaveSPP", JxtInfo(SPP_SYNCH_JXT_INTERFACE_ID,         NULL)}
                    // {"SearchFlt", JxtInfo(WEB_JXT_IFACE_ID,                 NULL)}
                }
            }
        };

        InetClient client_info;
        std::string operation;
        bool jxt_format;
        std::string exchange_type;
        std::string user_name;
        std::string password;
        std::string uri_path;
        httpParams uri_params;

        boost::optional<const JxtInfo &> get_jxt_info() const;
        std::string getQueryTagPropsString() const;
        static std::pair<std::string::size_type, std::string::size_type>
            findTag(const std::string& str,
                    std::string::size_type pos,
                    const std::string& tagName);
        void populate_client_from_uri(const std::string& uri);
    public:
        void clear()
        {
            client_info.clear();
            operation.clear();
            jxt_format = false;
            exchange_type.clear();
            user_name.clear();
            password.clear();
            uri_path.clear();
            uri_params.clear();
        }

        HTTPClient() { clear(); }

        void get(const ServerFramework::HTTP::request &req);
        void InitLogTime() const;


        std::string toString() const;
        bool toJXT( const ServerFramework::HTTP::request& req, std::string &header, std::string &body );
        ServerFramework::HTTP::reply& fromJXT( std::string res, ServerFramework::HTTP::reply& rep );
        JxtInfo::TPostProc get_post_proc() const;

};

void http_main(ServerFramework::HTTP::reply& rep, const ServerFramework::HTTP::request& req);

} //end namespace

#endif // __HTTP_MAIN_H__
