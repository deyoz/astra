#ifndef __HTTP_MAIN_H__
#define __HTTP_MAIN_H__

#include "jxtlib/JxtInterface.h"
#include <string>
#include "serverlib/http_parser.h"

namespace AstraHTTP
{

#define HTTP_JXT_IFACE_ID "HTTP"

struct HTTPClient
{
  std::string client_id;
  std::string operation;
  std::string pult;
  std::string user_name;
  std::string password;
  std::string toString();
  void toJXT( const ServerFramework::HTTP::request& req, std::string &header, std::string &body );
  ServerFramework::HTTP::reply& fromJXT( std::string res, ServerFramework::HTTP::reply& rep );
};


void http_main(ServerFramework::HTTP::reply& rep, const ServerFramework::HTTP::request& req);


class HTTPRequestsIface : public JxtInterface
{
public:
  HTTPRequestsIface() : JxtInterface("",HTTP_JXT_IFACE_ID)
  {
     Handler *evHandle;
     // Расширенный поиск пассажиров
     evHandle=JxtHandler<HTTPRequestsIface>::CreateHandler(&HTTPRequestsIface::SaveSPP);
     AddEvent("SaveSPP",evHandle);
  }
  void SaveSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode){};
};
} //end namespace


#endif // __HTTP_MAIN_H__

