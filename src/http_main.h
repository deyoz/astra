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
#define STAT_JXT_INTERFACE_ID "stat"
#define PIECE_CONCEPT_JXT_INTERFACE_ID "PieceConcept"

void HTTPPostProcessXMLAnswer();
void TlgPostProcessXMLAnswer();

struct JxtInfo {
  std::string interface;
  void (*post_proc)();
  JxtInfo(const std::string& name, void (*proc)()):
    interface(name), post_proc(proc) {}
  JxtInfo() : post_proc(NULL) {}
};

struct HTTPClient
{
  InetClient client_info;
  std::string operation;
  bool jxt_format;
  std::string user_name;
  std::string password;
  std::map<std::string, JxtInfo> jxt_interface;
  std::string toString();
  void toJXT( const ServerFramework::HTTP::request& req, std::string &header, std::string &body );
  ServerFramework::HTTP::reply& fromJXT( std::string res, ServerFramework::HTTP::reply& rep );
  HTTPClient() {
    jxt_format = false;
    jxt_interface["SaveSPP"] =      JxtInfo(SPP_SYNCH_JXT_INTERFACE_ID, NULL);
    jxt_interface["CrewCheckin"] =  JxtInfo(CHECKIN_JXT_INTERFACE_ID, HTTPPostProcessXMLAnswer);
    jxt_interface["tlg_srv"] =      JxtInfo(TELEGRAM_JXT_INTERFACE_ID, TlgPostProcessXMLAnswer);
    jxt_interface["kick"] =         JxtInfo(TELEGRAM_JXT_INTERFACE_ID, TlgPostProcessXMLAnswer);
    jxt_interface["stat_srv"] =     JxtInfo(STAT_JXT_INTERFACE_ID, NULL);
    jxt_interface["piece_concept"] =JxtInfo(PIECE_CONCEPT_JXT_INTERFACE_ID, NULL);
  }
};

HTTPClient getHTTPClient(const ServerFramework::HTTP::request& req);
void http_main(ServerFramework::HTTP::reply& rep, const ServerFramework::HTTP::request& req);

} //end namespace

#endif // __HTTP_MAIN_H__
