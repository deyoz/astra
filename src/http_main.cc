//#include <arpa/inet.h>
//#include <memory.h>
#include <string>
#include <vector>
#include <libxml/uri.h>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "oralib.h"
#include "astra_utils.h"
#include "http_main.h"
#include "astra_locale.h"
#include "serverlib/perfom.h"
#include "serverlib/ourtime.h"
#include "serverlib/str_utils.h"
#include "xml_unit.h"
#include "date_time.h"
#include "web_main.h"
#include "jxtlib/xml_stuff.h"
#include "jxtlib/jxtlib.h"
#include "astra_callbacks.h"
#include "xml_unit.h"
#include "qrys.h"

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
const std::string HOST = "Host";
const std::string AUTHORIZATION = "Authorization";

const std::string LOGIN = "login";
const std::string PASSWORD = "password";

using namespace ServerFramework::HTTP;

std::string HTTPClient::toString()
{
  string res = "client_id: " + client_info.client_id + ", operation=" + operation + ", user_name=" + user_name + ", password=" + password;
  return res;
}

void populate_client_from_uri(const request& req, HTTPClient& client)
{
    if ( req.uri.find("?") == std::string::npos ) return;
    // before "?"
    std::string uri = req.uri.substr( 0, req.uri.find("?") );
    ProgTrace( TRACE5, "%s: uri = %s", __FUNCTION__, uri.c_str() );
    std::vector<std::string> uri_vec;
    boost::split(uri_vec, uri, boost::is_any_of("/"));
    if ( uri_vec.size() >= 4 )
    {
        client.extra_params["FLIGHT"] = uri_vec[1];
        client.extra_params["DATE"] = uri_vec[2];
        client.extra_params["AIRP"] = uri_vec[3];
    }
    // after "?"
    std::string query = req.uri.substr( req.uri.find("?") + 1 );
    ProgTrace( TRACE5, "%s: query = %s", __FUNCTION__, query.c_str() );
    std::vector<std::string> query_vec;
    std::map<std::string, std::string> query_map;
    boost::split(query_vec, query, boost::is_any_of("&"));
    for ( std::vector<std::string>::iterator i_part = query_vec.begin(); i_part != query_vec.end(); ++i_part )
    {
        ProgTrace( TRACE5, "%s: %s", __FUNCTION__, i_part->c_str() );
        std::vector<std::string> part_vec;
        boost::split(part_vec, *i_part, boost::is_any_of("="));
        if ( part_vec.size() == 2 and !part_vec[0].empty() and !part_vec[1].empty() ) {
            query_map[part_vec[0]] = part_vec[1];
            client.extra_params[part_vec[0]] = part_vec[1];
        }
    }
    client.client_info = getInetClient(query_map[CLIENT_ID]);
    client.operation = query_map[OPERATION];
    client.user_name = query_map[LOGIN];
    client.password = query_map[PASSWORD];
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
  if (client.client_info.client_id.empty())
  {
    //ProgError(STDLOG, "%s: empty client_id", __FUNCTION__);
    ProgTrace( TRACE5, "%s: empty client_id, trying to populate HTTPClient from URI", __FUNCTION__ );
    populate_client_from_uri(req, client);
  }

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

bool isCR(char c) { return c == '\r'; };

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
  // �������⥫�� ���������, ����祭�� �� ࠧ��� URI
  for (std::map<std::string, std::string>::iterator i_extra = extra_params.begin(); i_extra != extra_params.end(); ++i_extra )
  {
      http_header += string("<param>") +
                   "<name>" + i_extra->first + "</name>\n" +
                   "<value>" + i_extra->second + "</value>\n" + "</param>\n";
  }
  http_header += "</header>\n";

  //reqInfoData.pr_web = (head[0]==2); //�⮡� pr_web ��⠭������ �  true
  header = (string(1,2) + string(44,0) + client_info.pult + "  " + client_info.client_id + string(100,0)).substr(0,100);

  if (!jxt_format) //��ࠬ��� http �����
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

         // remove any #13
         content.erase(remove_if(content.begin(), content.end(), isCR), content.end());

         bool is_xml = true;
         try {
             is_xml = XMLDoc(content).docPtr() != NULL;
         } catch(...) {
             is_xml = false;
         }

         if(is_xml) {
             body += content.c_str();
             body.insert( pos + sss.length(), string("<term><query id=") + "'" + jxt_interface[operation].interface + "' screen='AIR.exe' opr='" + CP866toUTF8(client_info.opr) + "'>" + http_header + "<content>\n" );
             body += string(" </content>\n") + "</" + operation + ">\n</query></term>";
         } else {
             body.insert( pos + sss.length(), string("<term><query id=") + "'" + jxt_interface[operation].interface + "' screen='AIR.exe' opr='" + CP866toUTF8(client_info.opr) + "'>" + http_header + "<content/>\n" );
             body += (string)"</" + operation + ">\n</query></term>";
             // screeneng chars such as ampersand, may be encountered in content (& -> &amp;)
             XMLDoc doc(body);
             string nodeName = "/term/query/" + operation;
             ReplaceTextChild(NodeAsNode(nodeName.c_str(), doc.docPtr()), "content", content);
             body = XMLTreeToText(doc.docPtr());
         }
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
      if(res[pos1 - 2] == '/') // �᫨ ����⨫�� <content/>
          res.erase();
      else {
          string::size_type pos2 = res.find( "</content>", pos );
          if ( pos1 != string::npos && pos2 != string::npos ) {
            res = res.substr( pos1, pos2-pos1 );
          }
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

void save_http_client_headers(const request &req)
{
    TCachedQuery Qry(
            "begin "
            "   insert into http_client_headers ( "
            "       client_id, "
            "       host, "
            "       path, "
            "       operation, "
            "       time "
            "   ) values ( "
            "       :client_id, "
            "       :host, "
            "       :path, "
            "       :operation, "
            "       :time "
            "   ); "
            "exception "
            "   when dup_val_on_index then "
            "       update http_client_headers "
            "           set time = :time "
            "       where "
            "           client_id = :client_id and "
            "           host = :host and "
            "           path = :path and "
            "           nvl(operation, ' ') = nvl(:operation, ' '); "
            "end; ",
        QParams()
            << QParam("client_id", otString)
            << QParam("host", otString)
            << QParam("path", otString, req.uri)
            << QParam("operation", otString)
            << QParam("time", otDate, NowUTC())
            );
    bool pr_kick = false;
    bool pr_client_id = false;
    for (request::Headers::const_iterator iheader=req.headers.begin(); iheader!=req.headers.end(); iheader++) {
        if ( iheader->name == CLIENT_ID ) {
            pr_client_id = not iheader->value.empty();
            Qry.get().SetVariable("client_id", iheader->value);
        }
        if ( iheader->name == HOST )
            Qry.get().SetVariable("host", iheader->value);
        if ( iheader->name == OPERATION ) {
            pr_kick = iheader->value == "kick";
            Qry.get().SetVariable("operation", iheader->value);
        }
    }
    if(not pr_kick and pr_client_id) Qry.get().Execute();
}

void http_main(reply& rep, const request& req)
{
  LogTrace(TRACE5) << "GRISHA: " << __FUNCTION__;
  LogTrace(TRACE5) << "method = " << req.method;
  LogTrace(TRACE5) << "uri = " << req.uri;
  LogTrace(TRACE5) << "content = " << req.content;
  try
  {
    try
    {
      HTTPClient client = getHTTPClient( req );
      save_http_client_headers(req);

      InitLogTime(client.client_info.pult.c_str());

      char *res = 0;
      int len = 0;
      static ServerFramework::ApplicationCallbacks *ac=
               ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks();

      string header, body;
      client.toJXT( req, header, body );
      ProgTrace( TRACE5, "body.size()=%zu, header.size()=%zu, len=%d", body.size(), header.size(), len );
      /*!!!*/ProgTrace( TRACE5, "body=%s", body.c_str() );

      AstraJxtCallbacks* astra_cb_ptr = dynamic_cast<AstraJxtCallbacks*>(jxtlib::JXTLib::Instance()->GetCallbacks());
      astra_cb_ptr->SetPostProcessXMLAnswerCallback(client.jxt_interface[client.operation].post_proc);

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
    LogTrace(TRACE5) << "TlgPostProcessXMLAnswer: resDoc: " << GetXMLDocText(xmlRC->resDoc);
    xmlNodePtr resNode = NodeAsNode("/term/answer",xmlRC->resDoc);
    if(resNode->children == NULL) {
        NewTextChild( resNode, "content");
    } else {
        std::string error_code, error_message;
        xmlNodePtr errNode = AstraLocale::selectPriorityMessage(resNode, error_code, error_message);

        if (errNode!=NULL)
        {
            xmlFreeNode(errNode);
            LogTrace(TRACE5) << "tlg_srv err: " << error_message.c_str();
            NewTextChild( resNode, "content", error_message);
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

