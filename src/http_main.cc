//#include <arpa/inet.h>
//#include <memory.h>
#include <string>
#include <vector>

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
#include "html_pages.h"
#include "http_consts.h"

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

using namespace ServerFramework::HTTP;

std::string HTTPClient::toString() const
{
  string res = "client_id: " + client_info.client_id + "," +
               "pult=" + client_info.pult + "," +
               "operation=" + operation + ", user_name=" + user_name + ", password=" + password;
  return res;
}

std::string HTTPClient::getQueryTagPropsString() const
{
    string interface;
    string opr = client_info.opr;
    auto jxt_info = get_jxt_info();
    if(jxt_info) interface = jxt_info->interface;
    return " id='" + interface + "' screen='AIR.EXE' opr='" + CP866toUTF8(opr) +"'";
}

std::pair<string::size_type, string::size_type> HTTPClient::findTag(const std::string& str,
                                                                    string::size_type pos,
                                                                    const std::string& tagName)
{
  string::size_type tagBegin, tagEnd;
  do
  {
    tagBegin=string::npos;
    tagEnd=string::npos;

    tagBegin=str.find("<"+tagName, pos);
    if (tagBegin!=string::npos)
    {
      tagEnd=str.find_first_of(" />", ++tagBegin);
      if (tagEnd!=string::npos) pos=tagEnd; //для возможной следующей итерации поиска
    }
  }
  while (!tagName.empty() &&
         tagBegin!=string::npos &&
         tagEnd!=string::npos &&
         str.substr(tagBegin, tagEnd-tagBegin)!=tagName);

  return make_pair(tagBegin, tagEnd);
}

void HTTPClient::populate_client_from_uri(const string& uri)
{
    LogTrace(TRACE5) << "populate_client_from_uri incoming uri: '" << uri << "'";
    if(uri_path.empty())
        uri_path = uri.substr( 0, uri.find("?") );
    ProgTrace( TRACE5, "%s: client.uri_path = %s", __FUNCTION__, uri_path.c_str() );
    // after "?"
    std::string query = uri.substr( uri.find("?") + 1 );
    ProgTrace( TRACE5, "%s: query = %s", __FUNCTION__, query.c_str() );
    std::vector<std::string> query_vec;
    boost::split(query_vec, query, boost::is_any_of("&"));
    for ( std::vector<std::string>::iterator i_part = query_vec.begin(); i_part != query_vec.end(); ++i_part )
    {
        ProgTrace( TRACE5, "%s: %s", __FUNCTION__, i_part->c_str() );
        std::vector<std::string> part_vec;
        boost::split(part_vec, *i_part, boost::is_any_of("="));
        if ( part_vec.size() == 1 and !part_vec[0].empty())
            uri_params[part_vec[0]];
        if ( part_vec.size() == 2 and !part_vec[0].empty() and !part_vec[1].empty() ) {
            uri_params[part_vec[0]] = part_vec[1];
        }
    }
    if(not uri_params[CLIENT_ID].empty()) {
        client_info = getInetClient(uri_params[CLIENT_ID]);
        operation = uri_params[OPERATION];
        if(operation.empty())
            operation = "get_resource";
        user_name = uri_params[LOGIN];
        password = uri_params[PASSWORD];
    }
}

void HTTPClient::get(const request& req)
{
  httpParams p; // без учета регистра
  p << req.headers;
  ProgTrace( TRACE5, "%s: %s", __FUNCTION__, p.trace().c_str() );
  if ( p.find( CLIENT_ID ) != p.end() ) {
    client_info = getInetClient( p[CLIENT_ID] );
  }
  if ( p.find( OPERATION ) != p.end() ) {
    operation =  p[OPERATION];
  }
  if ( p.find( AUTHORIZATION ) != p.end() && p[ AUTHORIZATION ].length() > 6 ) {
    string Authorization = p[ AUTHORIZATION ].substr( 6 );
    Authorization = StrUtils::b64_decode( Authorization );
    if ( Authorization.find( ":" ) != std::string::npos ) {
      user_name = Authorization.substr( 0, Authorization.find( ":" ) );
      password = Authorization.substr( Authorization.find( ":" ) + 1 );
    }
  }
  if (client_info.client_id.empty())
  {
    // Не удалось заполнить client из http headers
    // Пробуем из uri
    ProgTrace( TRACE5, "%s: empty client_id, trying to populate HTTPClient from URI", __FUNCTION__ );
    populate_client_from_uri(req.uri);
    if (client_info.client_id.empty()) {
        // Из uri не получилось
        // Пробуем из http header-а Referer
        if ( p.find( REFERER ) != p.end() ) {
          populate_client_from_uri(p[REFERER]);
        }
    }
  }
  if (client_info.client_id.empty()) { //запрос от киоска?
     if ( p.find(KIOSKID) != p.end() &&
          p.find(KIOSK_APPLICATION_NAME) != p.end() ) {
        client_info = getInetClientByKioskId( p[KIOSKID], p[KIOSK_APPLICATION_NAME] );
     }
  }


  if (client_info.client_id.empty()) ProgError(STDLOG, "%s: empty client_id", __FUNCTION__);

  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT http_user, http_pswd, exchange_type "
    "FROM http_clients "
    "WHERE id=:client_id";
  Qry.CreateVariable( "client_id", otString, client_info.client_id );
  Qry.Execute();
  if(not Qry.Eof)
      exchange_type = Qry.FieldAsString("exchange_type");
  if(exchange_type == EXCHANGE_TYPE::CUWS)
      operation = exchange_type;
  if (Qry.Eof ||
      user_name != Qry.FieldAsString( "http_user" ) ||
      password != Qry.FieldAsString( "http_pswd" ))
  {
  //ProgTrace(TRACE5, "%s=%s, %s=%s", user_name.c_str(), Qry.FieldAsString( "http_user" ), password.c_str(), Qry.FieldAsString( "http_pswd" ) );
      client_info.opr.clear();
      ProgError(STDLOG, "%s: wrong authorization (client_id=%s)",
                __FUNCTION__,
                client_info.client_id.c_str());
  }
  jxt_format = operation.empty();
  ProgTrace( TRACE5, "%s: %s", __FUNCTION__, toString().c_str() );
}

bool HTTPClient::toJXT( const ServerFramework::HTTP::request& req, std::string &header, std::string &body )
{
  header.clear();
  body.clear();
  string http_header = "<" + operation + ">\n<header>\n";
  for (request::Headers::const_iterator iheader=req.headers.begin(); iheader!=req.headers.end(); iheader++){
    http_header += string("<param>") +
                   "<name>" + iheader->name + "</name>\n" +
                   "<value>" + EncodeSpecialChars(iheader->value) + "</value>\n" + "</param>\n";
  }
  http_header += "</header>\n";
  http_header += "<get_params>\n";
  // параметры, полученные при разборе URI
  for (auto param : uri_params )
  {
      string unescaped = URIUnescapeString(param.second);
      http_header += string("<param>") +
                   "<name>" + param.first + "</name>\n" +
                   "<value>" + URIUnescapeString(param.second) + "</value>\n" + "</param>\n";
  }
  http_header += "</get_params>\n";
  http_header += "<uri_path>" + EncodeSpecialChars(uri_path) + "</uri_path>";

  //reqInfoData.pr_web = (head[0]==2); //чтобы pr_web установился в  true
  header = (string(1,2) + string(44,0) + client_info.pult + "  " + client_info.client_id + string(100,0)).substr(0,100);

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

         // remove any #13
         content.erase(remove_if(content.begin(), content.end(), [](char c){return c == '\r';}), content.end());

         bool is_xml = true;
         try {
             is_xml = XMLDoc(content).docPtr() != NULL;
         } catch(...) {
             is_xml = false;
         }

         if(is_xml) {
             body += content.c_str();
             body.insert(
                     pos + sss.length(),
                     string("<term><query") + getQueryTagPropsString() + ">" +
                     http_header + "<content>\n" );
             body += string(" </content>\n") + "</" + operation + ">\n</query></term>";
         } else {
             body.insert(
                     pos + sss.length(),
                     string("<term><query") + getQueryTagPropsString() + ">" +
                     http_header + "<content/>\n" );
             body += (string)"</" + operation + ">\n</query></term>";
             // screening chars such as ampersand, may be encountered in content (& -> &amp;)
             XMLDoc doc(body);
             if(not doc.docPtr())
                 throw Exception("toJXT wrong content");
             string nodeName = "/term/query/" + operation;
             ReplaceTextChild(NodeAsNode(nodeName.c_str(), doc.docPtr()), "content", content);
             body = XMLTreeToText(doc.docPtr());
         }
      }
  }
  else
  {
      body = req.content;
      auto queryPos=findTag(body, 0, "query");
      if (queryPos.first==string::npos ||
          queryPos.second==string::npos)
      {
        ProgTrace(TRACE1,"Unable to find <query> tag!");
        return true;
      }

      auto operationPos=findTag(body, queryPos.second, "");
      if (operationPos.first==string::npos ||
          operationPos.second==string::npos)
      {
        ProgTrace(TRACE1,"Unable to find operation tag!");
        return true;
      }

      operation=body.substr(operationPos.first, operationPos.second-operationPos.first);

      if (operation=="kick")
      {
        operation.clear();
        return true;
      }

      body.insert(queryPos.second, getQueryTagPropsString());
  }
  return get_jxt_info();
}

bool get_b64_prop(const string &val)
{
    bool result;
    result = val.find(" b64=\"1\"") != string::npos;
    return result;
}

reply& HTTPClient::fromJXT( std::string res, reply& rep )
{
  TResHTTPParams http_params;
  http_params.fromXML(res);

  bool b64 = false;
  if (!jxt_format)
  {
    string::size_type pos = res.find( "<content" );
    if ( pos != string::npos) {
      string::size_type pos1 = res.find( ">", pos) + 1;
      if(res[pos1 - 2] == '/') // если встретился <content/>
          res.erase();
      else {
          string::size_type pos2 = res.find( "</content>", pos );
          if ( pos1 != string::npos && pos2 != string::npos ) {
            b64 = get_b64_prop(res.substr(pos, pos1 - pos));
            res = res.substr( pos1, pos2-pos1 );
          }
      }
    }
  }

  if(b64)
      rep.content = StrUtils::b64_decode(res);
  else {
      XMLDoc doc(res);
      if(doc.docPtr()) {
          // отформатировать отступы в XML
          res = XMLTreeToText(doc.docPtr());
      }
      rep.content = /*"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +*/ res;
  }

  rep.status = http_params.status;

  rep.headers.clear();
  rep.headers.push_back(header());
  rep.headers.back().name = "Content-Length";
  rep.headers.back().value = boost::lexical_cast<std::string>(rep.content.size());
  rep.headers.push_back(header());
  rep.headers.back().name = "Content-Type";
  rep.headers.push_back(header());
  rep.headers.back().name = "Access-Control-Allow-Origin";
  rep.headers.back().value = "*";
  rep.headers.push_back(header());
  rep.headers.back().name = "Access-Control-Allow-Headers";
  rep.headers.back().value = "CLIENT-ID,OPERATION,Authorization";
  rep.headers.push_back(header());
  rep.headers.back().name = "Cache-Control";
  rep.headers.back().value = "no-cache";

  for(map<string, string>::const_iterator i = http_params.hdrs.begin();
          i != http_params.hdrs.end(); i++) {
      rep.headers.push_back(header());
      rep.headers.back().name = i->first;
      rep.headers.back().value = i->second;
  }

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
    httpParams p; // без учета регистра
    p << req.headers;
    if ( p.find(CLIENT_ID) != p.end() ) {
      pr_client_id = not  p[CLIENT_ID].empty();
      Qry.get().SetVariable("client_id", p[CLIENT_ID]);
    }
    if ( p.find(HOST) != p.end() ) {
      Qry.get().SetVariable("host", p[HOST]);
    }
    if ( p.find(OPERATION) != p.end() ) {
      pr_kick = p[OPERATION] == "kick";
      Qry.get().SetVariable("operation", p[OPERATION]);
    }
    if(not pr_kick and pr_client_id) Qry.get().Execute();
}

void http_main(reply& rep, const request& req)
{
    LogTrace(TRACE5) << "HTTP REQUEST: '" << req.to_string();
    try
    {
        try
        {
            HTTPClient client;
            client.get(req);
            save_http_client_headers(req);

            client.InitLogTime();

            char *res = 0;
            int len = 0;
            static ServerFramework::ApplicationCallbacks *ac=
                ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks();

            string header, body;
            if(client.toJXT( req, header, body )) {
                ProgTrace( TRACE5, "body.size()=%zu, header.size()=%zu, len=%d", body.size(), header.size(), len );
                /*!!!*/ProgTrace( TRACE5, "body=%s", body.c_str() );

                AstraJxtCallbacks* astra_cb_ptr = dynamic_cast<AstraJxtCallbacks*>(jxtlib::JXTLib::Instance()->GetCallbacks());
                astra_cb_ptr->SetPostProcessXMLAnswerCallback(client.get_post_proc());

                int newlen=ac->jxt_proc((const char *)body.data(),body.size(),(const char *)header.data(),header.size(), &res, len);
                ProgTrace( TRACE5, "newlen=%d, len=%d, header.size()=%zu", newlen, len, header.size() );
                body = string( res + header.size(), newlen - header.size() );
                client.fromJXT( body, rep );
            } else {
                rep.status = reply::forbidden;
                rep.headers.resize(2);
                rep.headers[0].name = "Content-Length";
                rep.content = "FORBIDDEN!";
                rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
                rep.headers[1].name = "Content-Type";
            }
        }
        catch(std::exception &e)
        {
            ProgError(STDLOG, "%s: exception: %s", __FUNCTION__, e.what());
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
    LogTrace(TRACE5) << "http_main: finished uri = " << req.uri;
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

void CrewPostProcessXMLAnswer()
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

void ZamarPostProcessXMLAnswer()
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  XMLRequestCtxt *xmlRC = getXmlCtxt();
  xmlNodePtr resNode = NodeAsNode("/term/answer",xmlRC->resDoc);
  xmlNodePtr reqNode = NodeAsNode("/term/query",xmlRC->reqDoc);

//  LogTrace(TRACE5) << __func__ << " GetXMLDocText: " << GetXMLDocText(xmlRC->resDoc);

  std::string error_code, error_message;
  xmlNodePtr errNode = AstraLocale::selectPriorityMessage(resNode, error_code, error_message);

  if (reqNode!=nullptr && reqNode->children!=nullptr)
  {
    NewTextChild( resNode, "queryType", (const char*)reqNode->children->name );
    SetProp( resNode, "queryType", (const char*)reqNode->children->name );
  }

  if (errNode!=NULL)
  {
    if (strcmp((const char*)errNode->name,"error")==0 ||
        strcmp((const char*)errNode->name,"checkin_user_error")==0 ||
        strcmp((const char*)errNode->name,"user_error")==0)
    {
      NewTextChild( resNode, "command", error_code );
      NewTextChild( resNode, "error", error_message );
    };
    xmlFreeNode(errNode);
  }
}

boost::optional<const JxtInfo &> HTTPClient::get_jxt_info() const
{
    boost::optional<const JxtInfo &> result;
    try {
        result = jxt_interface.at(exchange_type).at(operation);
    } catch(out_of_range &) {
    }
    return result;
}

void HTTPClient::InitLogTime() const
{
    ::InitLogTime(client_info.pult.c_str());
}

JxtInfo::TPostProc HTTPClient::get_post_proc() const
{
    JxtInfo::TPostProc result = nullptr;
    auto jxt_info = get_jxt_info();
    if(jxt_info) result = jxt_info->post_proc;
    return result;
}

} //end namespace AstraHTTP

/////////////////////////////////////////////////////////////////////////////////////////////////////////

