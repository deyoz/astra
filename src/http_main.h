#ifndef __HTTP_MAIN_H__
#define __HTTP_MAIN_H__

#include <string>
#include <map>
#include "serverlib/http_parser.h"
#include "web_main.h"

namespace AstraHTTP
{

#define SPP_SYNCH_JXT_INTERFACE_ID "SppSynch"
#define CHECKIN_JXT_INTERFACE_ID "CheckIn"
#define TELEGRAM_JXT_INTERFACE_ID "Telegram"
#define KIOSK_ALIAS_JXT_INTERFACE_ID "KioskAlias"
#define KIOSK_REQUEST_JXT_INTERFACE_ID "KioskRequest"
#define STAT_JXT_INTERFACE_ID "stat"
#define HTML_JXT_INTERFACE_ID "html"
#define PIECE_CONCEPT_JXT_INTERFACE_ID "PieceConcept"
#define PRINT_JXT_INTERFACE_ID "print"
#define ZAMAR_DSM_JXT_INTERFACE_ID "ZamarDSM"

struct HTTPClient;

void HTTPPostProcessXMLAnswer();
void TlgPostProcessXMLAnswer();

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

struct HTTPClient
{
  InetClient client_info;
  std::string operation;
  bool jxt_format;
  std::string user_name;
  std::string password;
  std::string uri_path;
  std::map<std::string, JxtInfo> jxt_interface;
  httpParams uri_params;
  std::string toString();
  void toJXT( const ServerFramework::HTTP::request& req, std::string &header, std::string &body );
  ServerFramework::HTTP::reply& fromJXT( std::string res, ServerFramework::HTTP::reply& rep );
  HTTPClient() {
    jxt_format = false;
    jxt_interface["SaveSPP"] =              JxtInfo(SPP_SYNCH_JXT_INTERFACE_ID,     NULL);
    jxt_interface["SaveSinhronSPP"] =       JxtInfo(SPP_SYNCH_JXT_INTERFACE_ID,     NULL);
    jxt_interface["CREWCHECKIN"] =          JxtInfo(CHECKIN_JXT_INTERFACE_ID,       HTTPPostProcessXMLAnswer);
    jxt_interface["tlg_srv"] =              JxtInfo(TELEGRAM_JXT_INTERFACE_ID,      TlgPostProcessXMLAnswer);
    jxt_interface["kick"] =                 JxtInfo(TELEGRAM_JXT_INTERFACE_ID,      TlgPostProcessXMLAnswer);
    jxt_interface["kuf_file"] =             JxtInfo(TELEGRAM_JXT_INTERFACE_ID,      NULL);
    jxt_interface["kuf_stat"] =             JxtInfo(TELEGRAM_JXT_INTERFACE_ID,      NULL);
    jxt_interface["kuf_stat_flts"] =        JxtInfo(TELEGRAM_JXT_INTERFACE_ID,      NULL);
    jxt_interface["EventToServer"] =        JxtInfo(KIOSK_REQUEST_JXT_INTERFACE_ID, NULL);
    jxt_interface["ViewCraft"] =            JxtInfo(KIOSK_REQUEST_JXT_INTERFACE_ID, NULL);
    jxt_interface["kiosk_alias"] =          JxtInfo(KIOSK_ALIAS_JXT_INTERFACE_ID,   NULL);
    jxt_interface["kiosk_alias_locale"] =   JxtInfo(KIOSK_ALIAS_JXT_INTERFACE_ID,   NULL);
    jxt_interface["stat_srv"] =             JxtInfo(STAT_JXT_INTERFACE_ID,          NULL);
    jxt_interface["piece_concept"] =        JxtInfo(PIECE_CONCEPT_JXT_INTERFACE_ID, NULL);
    jxt_interface["GetGRPPrintData"] =      JxtInfo(PRINT_JXT_INTERFACE_ID,         NULL);
    // Запрос с одинаковым названием, но в разных классах
    // данная мэпа не подходит для такого случая
    // т.к. затирается обработчик
    // jxt_interface["GetPrintDataBP"] =       JxtInfo(PRINT_JXT_INTERFACE_ID,         NULL);
    jxt_interface["GetPrintDataBP"] =       JxtInfo(WEB_JXT_IFACE_ID,               NULL);
    jxt_interface["SearchFlt"] =            JxtInfo(WEB_JXT_IFACE_ID,               NULL);
    jxt_interface["GetGRPPrintDataBP"] =    JxtInfo(PRINT_JXT_INTERFACE_ID,         NULL);
    jxt_interface["GetImg"] =               JxtInfo(PRINT_JXT_INTERFACE_ID,         NULL);
    jxt_interface["get_resource"] =         JxtInfo(HTML_JXT_INTERFACE_ID,          NULL);
    jxt_interface["print_bp"] =             JxtInfo(PRINT_JXT_INTERFACE_ID,         NULL);
    jxt_interface["print_bp2"] =            JxtInfo(PRINT_JXT_INTERFACE_ID,         NULL);
    jxt_interface["PassengerSearch"] =      JxtInfo(ZAMAR_DSM_JXT_INTERFACE_ID,     NULL);
  }
};

HTTPClient getHTTPClient(const ServerFramework::HTTP::request& req);
void http_main(ServerFramework::HTTP::reply& rep, const ServerFramework::HTTP::request& req);

} //end namespace

#endif // __HTTP_MAIN_H__
