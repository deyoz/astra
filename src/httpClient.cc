#include "httpClient.h"
#include "file_queue.h"
#include "astra_service.h"
#include "astra_utils.h"
#include "basic.h"
#include "serverlib/str_utils.h"
#include <pion/http/parser.hpp>
#include <fstream>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdlib.h>

#define NICKNAME "ANNA"
#include "serverlib/test.h"
#include "exceptions.h"

using namespace EXCEPTIONS;

const std::string PARAM_URL = "URL";
const std::string PARAM_ACTION_CODE = "ACTION_CODE";
const std::string PARAM_LOGIN = "LOGIN";
const std::string PARAM_PASSWORD = "PASSWORD";

void Client::handle_connect(const boost::system::error_code& error,
                    boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
  if (req_info_.using_ssl) {
    if (!error)
    {
      ssl_socket_.async_handshake(boost::asio::ssl::stream_base::client,
          boost::bind(&Client::handle_handshake, this,
            boost::asio::placeholders::error));
    }
    else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
    {
      ssl_socket_.lowest_layer().close();
      boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      ssl_socket_.lowest_layer().async_connect(endpoint,
          boost::bind(&Client::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
    }
    else
      throw Exception("Connect failed: %s", error.message().c_str());
  }
  else handle_handshake(error);
}

void Client::handle_handshake(const boost::system::error_code& error)
{
  if (!error)
  {
    std::ostringstream ns_str;
    ns_str << "POST " << req_info_.path << " HTTP/1.1\r\n";
    ns_str << "Authorization: Basic " << StrUtils::b64_encode(req_info_.login + ":" + req_info_.pswd) << "\r\n";
    ns_str << "Content-length: " << req_info_.content.length() << "\r\n";
    ns_str << "SOAPAction: " << req_info_.action << "\r\n";
    ns_str << "Host: " << req_info_.host << ":" << req_info_.port << "\r\n";
    ns_str << "Content-type: text/xml; charset=\"UTF-8\"\r\n";
    ns_str << "\r\n";
    ns_str << req_info_.content;

    request_ = ns_str.str();
    if (req_info_.using_ssl) {
    boost::asio::async_write(ssl_socket_, boost::asio::buffer(request_),
      boost::bind(&Client::handle_write, this, boost::asio::placeholders::error));
    }
    else {
      boost::asio::async_write(socket_, boost::asio::buffer(request_),
        boost::bind(&Client::handle_write, this, boost::asio::placeholders::error));
    }
  }
  else
    throw Exception("Handshake failed: %s", error.message().c_str());
}

void Client::handle_write(const boost::system::error_code& error)
{
  if (!error)
  {
    if (req_info_.using_ssl) {
      boost::asio::async_read_until(ssl_socket_, reply_, "<?xml",
        boost::bind(&Client::handle_read_header, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    }
    else {
      boost::asio::async_read_until(socket_, reply_, "<?xml",
        boost::bind(&Client::handle_read_header, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    }
  }
  else
    throw Exception("Write failed: %s", error.message().c_str());
}

void Client::handle_read_header(const boost::system::error_code& error, size_t bytes_transferred)
{
  if (!error)
  {
    std::istream response_stream(&reply_);
    std::string http_version;
    response_stream >> http_version;
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      throw Exception("Invalid response");

    uint32_t status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (status_code != 200)    // 200 = status code OK
      throw Exception("Response returned with status code %d", status_code);

    reply_.consume(bytes_transferred - 2*http_version.size() - status_message.size() - 2);
    if(req_info_.using_ssl) {
      boost::asio::async_read(ssl_socket_, reply_, boost::asio::transfer_at_least(1),
        boost::bind(&Client::handle_read_body, this,
          boost::asio::placeholders::error));
    }
    else {
      boost::asio::async_read(socket_, reply_, boost::asio::transfer_at_least(1),
        boost::bind(&Client::handle_read_body, this,
          boost::asio::placeholders::error));
    }
  }
  else
    throw Exception("Read failed: %s", error.message().c_str());
}

void Client::handle_read_body(const boost::system::error_code& error)
{
  if (!error)
  {
    std::stringstream ss;
    ss << &reply_;
    std::string result = ss.str();
    process_reply(result);    
  }
  else {
    throw Exception("Read failed: %s", error.message().c_str());
  }
}

int httpClient_main(RequestInfo& request)
{
  try
  {
    boost::asio::io_service io_service;

    boost::asio::ip::tcp::resolver resolver(io_service);

    boost::asio::ip::tcp::resolver::query query(request.host, IntToString(request.port));
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    boost::asio::ssl::context ctx(io_service, boost::asio::ssl::context::sslv23_client);
    ctx.set_verify_mode(boost::asio::ssl::context::verify_none);

    Client c(io_service, ctx, iterator, request);

    io_service.run();
  }
  catch (std::exception& e)
  {
    throw Exception("httpClient_main: %s",e.what());
  }
  return 0;
}

void send_apis_tr()
{
  TFileQueue file_queue;
  file_queue.get( *TApisTRFilter::Instance() );
  for ( TFileQueue::iterator item=file_queue.begin(); item!=file_queue.end(); item++) {
      if ( item->params.find( PARAM_URL ) == item->params.end() ||
              item->params[ PARAM_URL ].empty() )
          throw Exception("url not specified");
      if ( item->params.find( PARAM_ACTION_CODE ) == item->params.end() ||
              item->params[ PARAM_ACTION_CODE ].empty() )
          throw Exception("action_code not specified");
      if ( item->params.find( PARAM_LOGIN ) == item->params.end() ||
              item->params[ PARAM_LOGIN ].empty() )
          throw Exception("login not specified");
      if ( item->params.find( PARAM_PASSWORD ) == item->params.end() ||
              item->params[ PARAM_PASSWORD ].empty() )
          throw Exception("password not specified");
      RequestInfo request;
      std::string proto;
      std::string query;
      if(not pion::http::parser::parse_uri(item->params[PARAM_URL], proto, request.host, request.port, request.path, query))
        throw Exception("parse_uri failed for '%s'", item->params[PARAM_URL].c_str());
      request.action = item->params[PARAM_ACTION_CODE];
      request.login = item->params[PARAM_LOGIN];
      request.pswd = item->params[PARAM_PASSWORD];
      request.content = item->data;
      request.using_ssl = (proto=="https")?true:false;
      TFileQueue::sendFile(item->id);
      httpClient_main(request);
      TFileQueue::doneFile(item->id);
      createMsg( *item, evCommit );
  }
}

void process_reply(const std::string& result)
{
  if(!result.empty()) {
    xmlDocPtr doc = NULL;
    try {
      doc = TextToXMLTree(result);
    }
    catch(...) { }
    if(doc != NULL) {
      try {
        xmlNodePtr rootNode=xmlDocGetRootElement(doc);
        xmlNodePtr node = rootNode->children;
        node = NodeAsNodeFast("Body", node);
        if(node) {
          node = node->children;
          node = NodeAsNodeFast("getFlightMessageResponse", node);
          if(node) {
            node = node->children;
            node = NodeAsNodeFast("Statu", node);
            if(node) {
              node = node->children;
              node = NodeAsNodeFast("explanation", node);
              std::string status = (node ? NodeAsString(node) : "");
              if(status != "OK") {
                ProgTrace(TRACE5, "%s", GetXMLDocText(doc->doc).c_str());
                  throw Exception("Return status not OK: '%s'", status.c_str());
              }
            }
          } else
            throw Exception("getFlightMessageResponse tag not found");
        } else
          throw Exception("Body tag not found");
      } catch(...) {
        xmlFreeDoc(doc);
        ProgTrace(TRACE5, "Reply: %s", result.c_str());
        throw;
      }
      xmlFreeDoc(doc);
    } else
      throw Exception("wrong answer XML");
  } else
    throw Exception("result is empty");
}
