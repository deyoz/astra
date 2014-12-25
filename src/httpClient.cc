#include "httpClient.h"
#include "file_queue.h"
#include "astra_utils.h"
#include "basic.h"
#include <pion/http/parser.hpp>
#include <fstream>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdlib.h>

#define NICKNAME "ANNA"
#include "serverlib/test.h"
#include "exceptions.h"

using namespace EXCEPTIONS;

const std::string APIS_TR = "APIS_TR";
const std::string PARAM_URL = "URL";
const std::string PARAM_ACTION_CODE = "ACTION_CODE";
const std::string PARAM_LOGIN = "LOGIN";
const std::string PARAM_PASSWORD = "PASSWORD";

std::string base64_encode(const std::string &s);

void Client::handle_connect(const boost::system::error_code& error,
                    boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
  if (!error)
  {
    socket_.async_handshake(boost::asio::ssl::stream_base::client,
        boost::bind(&Client::handle_handshake, this,
          boost::asio::placeholders::error));
  }
  else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
  {
    socket_.lowest_layer().close();
    boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
    socket_.lowest_layer().async_connect(endpoint,
        boost::bind(&Client::handle_connect, this,
          boost::asio::placeholders::error, ++endpoint_iterator));
  }
  else
  {
    ProgTrace(TRACE5,"Connect failed: %s", error.message().c_str());
  }
}

void Client::handle_handshake(const boost::system::error_code& error)
{
  if (!error)
  {
    std::fstream f;
    f.open("apis.xml", std::ios::out);
    f << req_info_.content;

    std::ostringstream ns_str;
    ns_str << "POST " << req_info_.path << " HTTP/1.1\r\n";
    ns_str << "Authorization: Basic '" << base64_encode(req_info_.login + ":" + req_info_.pswd) << "'\r\n";
    ns_str << "Content-length: " << req_info_.content.length() << "\r\n";
    ns_str << "SOAPAction: " << req_info_.action << "\r\n";
    ns_str << "Host: " << req_info_.host << ":" << req_info_.port << "\r\n";
    ns_str << "Content-type: text/xml; charset=\"UTF-8\"\r\n";
    ns_str << "\r\n";
    ns_str << req_info_.content;

    request_ = ns_str.str();
    ProgTrace(TRACE5,"Request:\n\n\n%s\n\n\n", request_.c_str());
    boost::asio::async_write(socket_,
        boost::asio::buffer(request_),
        boost::bind(&Client::handle_write, this,
          boost::asio::placeholders::error));
  }
  else
  {
    ProgTrace(TRACE5,"Handshake failed: %s", error.message().c_str());
  }
}

void Client::handle_write(const boost::system::error_code& error)
{
  if (!error)
  {
    ProgTrace(TRACE5, "handle_write");
    boost::asio::async_read_until(socket_, reply_, '\n',
        boost::bind(&Client::handle_read, this,
          boost::asio::placeholders::error));
  }
  else
  {
    ProgTrace(TRACE5, "Write failed: %s", error.message().c_str());
  }
}

void Client::handle_read(const boost::system::error_code& error)
{
  ProgTrace(TRACE5, "handle_read");
  if (!error)
  {
    std::stringstream ss;
    ss << &reply_;
    std::string res = ss.str();
    ProgTrace(TRACE5, "Reply: %s", res.c_str());
  }
  else
  {
    ProgTrace(TRACE5, "Read failed: %s", error.message().c_str());
  }
}

int httpClient_main(RequestInfo& request)
{
  ProgTrace(TRACE5, "send_apis started");
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
    ProgTrace(TRACE5, "send_apis completed");
  }
  catch (std::exception& e)
  {
    throw Exception("send_apis: %s",e.what());
  }

  return 0;
}

std::string base64_encode( const std::string &str ){

    BIO *base64_filter = BIO_new( BIO_f_base64() );
    BIO_set_flags( base64_filter, BIO_FLAGS_BASE64_NO_NL );

    BIO *bio = BIO_new( BIO_s_mem() );
    BIO_set_flags( bio, BIO_FLAGS_BASE64_NO_NL );

    bio = BIO_push( base64_filter, bio );

    BIO_write( bio, str.c_str(), str.length() );

    BIO_flush( bio );

    char *new_data;

    long bytes_written = BIO_get_mem_data( bio, &new_data );

    std::string result( new_data, bytes_written );
    BIO_free_all( bio );

    return result;
}

void send_apis_tr()
{
  ProgTrace(TRACE5,"send_apis_tr started");
  TFileQueue file_queue;
  file_queue.get( TFilterQueue( OWN_POINT_ADDR(), APIS_TR ) );
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
      httpClient_main(request);
      TFileQueue::deleteFile(item->id);
      ProgTrace(TRACE5, "send_apis_tr: id = %d completed", item->id);
  }
}
