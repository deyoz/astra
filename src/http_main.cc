//#include <arpa/inet.h>
//#include <memory.h>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>
#include "oralib.h"
#include "astra_utils.h"
#include "http_main.h"
#include "astra_locale.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/str_utils.h"
#include "xml_unit.h"
#include "basic.h"
#include "web_main.h"
#include "jxtlib/xml_stuff.h"
#include "jxtlib/jxtlib.h"
#include "astra_callbacks.h"
#include "xml_unit.h"

using namespace EXCEPTIONS;

//#include "serverlib/query_runner.h"
#include "serverlib/http_parser.h"
#include "serverlib/query_runner.h"

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
//using namespace ASTRA;
//using namespace ServerFramework;

namespace AstraHTTP
{

const std::string CLIENT_ID = "CLIENT-ID";
const std::string OPERATION = "OPERATION";
const std::string AUTHORIZATION = "Authorization";

using namespace ServerFramework::HTTP;

std::string HTTPClient::toString()
{
  string res = "client_id: " + client_info.client_id + ", operation=" + operation + ", user_name=" + user_name + ", password=" + password;
  return res;
}

HTTPClient getHTTPClient(const request& req)
{
  HTTPClient client;
  for (request::Headers::const_iterator iheader=req.headers.begin(); iheader!=req.headers.end(); iheader++) {
    ProgTrace( TRACE5, "%s: header: name=%s, value=%s", __FUNCTION__, iheader->name.c_str(), iheader->value.c_str() );
    if ( iheader->name == CLIENT_ID ) {
        client.client_info = getInetClient(iheader->value);
    }
    if ( iheader->name == OPERATION ) {
      client.operation = iheader->value;
    }
    if ( iheader->name == AUTHORIZATION && iheader->value.length() > 6 ) {
      string Authorization = iheader->value.substr( 6 );
      Authorization = StrUtils::b64_decode( Authorization );
      if ( Authorization.find( ":" ) != std::string::npos ) {
        client.user_name = Authorization.substr( 0, Authorization.find( ":" ) );
        client.password = Authorization.substr( Authorization.find( ":" ) + 1 );
      }
    }
  }
  if (client.client_info.client_id.empty()) ProgError(STDLOG, "%s: empty client_id", __FUNCTION__);

  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT http_user, http_pswd "
    "FROM http_clients "
    "WHERE id=:client_id";
  Qry.CreateVariable( "client_id", otString, client.client_info.client_id );
  Qry.Execute();
  if (Qry.Eof ||
      client.user_name != Qry.FieldAsString( "http_user" ) ||
      client.password != Qry.FieldAsString( "http_pswd" ))
  {
      client.client_info.opr.clear();
      ProgError(STDLOG, "%s: wrong authorization (client_id=%s)",
                __FUNCTION__,
                client.client_info.client_id.c_str());
  }
  if (client.operation.empty()) client.jxt_format = true;
  ProgTrace( TRACE5, "%s: %s", __FUNCTION__, client.toString().c_str() );
  return client;
}

void HTTPClient::toJXT( const ServerFramework::HTTP::request& req, std::string &header, std::string &body )
{
  header.clear();
  body.clear();
  string http_header = "<" + operation + ">\n<header>\n";
  for (request::Headers::const_iterator iheader=req.headers.begin(); iheader!=req.headers.end(); iheader++){
    http_header += string("<param>") +
                   "<name>" + iheader->name + "</name>\n" +
                   "<value>" + iheader->value + "</value>\n" + "</param>\n";
  }
  http_header += "</header>\n";

  //reqInfoData.pr_web = (head[0]==2); //чтобы pr_web установился в  true
  header = (string(1,2) + string(44,0) + client_info.pult + "  " + client_info.client_id + string(100,0)).substr(0,100);

  LogTrace(TRACE5) << "request content: " << req.content;

  if (!jxt_format) //параметр http запроса
  {
      string sss( "?>" );
      string::size_type pos = req.content.find( sss );

      if ( pos == string::npos ) { //only text
          body = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
          pos = body.find( sss );
      }
      if ( pos != string::npos ) {
         string content = req.content;
         char str_BOM[4];
         str_BOM[0] = 0xEF;
         str_BOM[1] = 0xBB;
         str_BOM[2] = 0xBF;
         str_BOM[3] = 0x00;
         if ( content.size() > 3 && content.substr(0,3) == str_BOM )
           content.erase( 0, 3 );
         body += content.c_str();
         body.insert( pos + sss.length(), string("<term><query id=") + "'" + jxt_interface[operation].interface + "' screen='AIR.exe' opr='" + CP866toUTF8(client_info.opr) + "'>" + http_header + "<content>\n" );
         body += string(" </content>\n") + "</" + operation + ">\n</query></term>";
      }
  }
  else
  {
      body += req.content.c_str();
      string sss("<query");
      string::size_type pos=req.content.find(sss);

      if(pos!=string::npos)
      {
          string::size_type pos1=req.content.find("<", pos+1) + 1;
          string::size_type pos2=req.content.find(">", pos1);
          operation = req.content.substr(pos1, pos2-pos1);
          body=body.substr(0,pos+sss.size())+" id='"+jxt_interface[operation].interface+"' screen='AIR.EXE' opr='"+ CP866toUTF8(client_info.opr) +"'"+body.substr(pos+sss.size());
      }
      else
          ProgTrace(TRACE1,"Unable to find <query> tag!");
  }
}

reply& HTTPClient::fromJXT( std::string res, reply& rep )
{
  if (!jxt_format)
  {
    string::size_type pos = res.find( "<content" );
    if ( pos != string::npos) {
      string::size_type pos1 = res.find( ">", pos) + 1;
      if(res[pos1 - 2] == '/') // если встретился <content/>
          res.erase();
      else {
          string::size_type pos2 = res.find( "</content>", pos );
          if ( pos1 != string::npos && pos2 != string::npos )
            res = res.substr( pos1, pos2-pos1 );
      }
    }
  }
  rep.status = reply::ok;
  rep.content = /*"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +*/ res;
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers[1].name = "Content-Type";
  return rep;
}

void http_main(reply& rep, const request& req)
{
  try
  {
    try
    {
      HTTPClient client = getHTTPClient( req );

      InitLogTime(client.client_info.pult.c_str());

      char *res = 0;
      int len = 0;
      static ServerFramework::ApplicationCallbacks *ac=
               ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks();

      string header, body;
      client.toJXT( req, header, body );
      ProgTrace( TRACE5, "body.size()=%zu, header.size()=%zu, len=%d", body.size(), header.size(), len );

      AstraJxtCallbacks* astra_cb_ptr = dynamic_cast<AstraJxtCallbacks*>(jxtlib::JXTLib::Instance()->GetCallbacks());
      astra_cb_ptr->SetPostProcessXMLAnswerCallback(client.jxt_interface[client.operation].post_proc);

      LogTrace(TRACE5) << "body: '" << body << "'";

      int newlen=ac->jxt_proc((const char *)body.data(),body.size(),(const char *)header.data(),header.size(), &res, len);
      ProgTrace( TRACE5, "newlen=%d, len=%d, header.size()=%zu", newlen, len, header.size() );
      body = string( res + header.size(), newlen - header.size() );
      client.fromJXT( body, rep );
    }
    catch(Exception &e)
    {
      ProgError(STDLOG, "%s: Exception: %s", __FUNCTION__, e.what());
      throw;
    }
    catch(...)
    {
      ProgError(STDLOG, "%s: Unknown error", __FUNCTION__);
      throw;
    }
  }
  catch(...)
  {
    rep.status = reply::internal_server_error;
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.content = "SERVER ERROR! CONTACT WITH DEVELOPERS";
    rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
    rep.headers[1].name = "Content-Type";
  }
  InitLogTime(NULL);
  return;
}

void TlgPostProcessXMLAnswer()
{
    ProgTrace(TRACE5, "%s started", __FUNCTION__);

    XMLRequestCtxt *xmlRC = getXmlCtxt();
    xmlNodePtr resNode = NodeAsNode("/term/answer",xmlRC->resDoc);
    if(resNode->children == NULL) {
        NewTextChild( resNode, "content");
    } else {
        std::string error_code, error_message;
        xmlNodePtr errNode = AstraLocale::selectPriorityMessage(resNode, error_code, error_message);

        if (errNode!=NULL)
        {
            xmlFreeNode(errNode);
            ProgError(STDLOG, "tlg_srv err: '%s'", error_message.c_str());
            NewTextChild( resNode, "content", INTERNAL_SERVER_ERROR);
        }
    }
}

void HTTPPostProcessXMLAnswer()
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer",xmlRC->resDoc);
  const char* operation = (const char*)xmlRC->reqDoc->children->children->children->name;

  std::string error_code, error_message;
  xmlNodePtr errNode = AstraLocale::selectPriorityMessage(resNode, error_code, error_message);

  resNode = NewTextChild( resNode, operation );

  if (errNode!=NULL)
  {
    NewTextChild( resNode, "proc_status", "ERROR" );

    if (strcmp((const char*)errNode->name,"error")==0 ||
        strcmp((const char*)errNode->name,"checkin_user_error")==0 ||
        strcmp((const char*)errNode->name,"user_error")==0)
    {
      NewTextChild( resNode, "error_code", error_code );
      NewTextChild( resNode, "error_message", error_message );
    };

    xmlFreeNode(errNode);
  }
  else
    NewTextChild(resNode, "proc_status", "OK");
}

} //end namespace AstraHTTP

/////////////////////////////////////////////////////////////////////////////////////////////////////////

