#include "httpClient.h"
#include "file_queue.h"
#include "astra_service.h"
#include "astra_utils.h"
#include "basic.h"
#include "serverlib/str_utils.h"
#include <pion/http/parser.hpp>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
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


class Client
{
private:
  RequestInfo req_info_;
  ResponseInfo res_info_;
  std::string request_;
  boost::asio::streambuf reply_;
  boost::asio::deadline_timer deadline_;
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::ssl::context ctx;
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_;
  boost::asio::ip::tcp::socket socket_;
  //bool would_block_;

public:
  Client(boost::asio::io_service& io_service, const RequestInfo& request)
    : req_info_(request), deadline_(io_service),  resolver_(io_service),
      ctx(io_service, boost::asio::ssl::context::sslv23_client),
      ssl_socket_(io_service, ctx), socket_(io_service) {
    //  would_block_(true) {

    ctx.set_verify_mode(boost::asio::ssl::context::verify_none); //??? возможно надо делать перед конструктором ssl_socket_???
    res_info_.clear();
    deadline_.expires_from_now(boost::posix_time::milliseconds(req_info_.timeout));
    deadline_.async_wait(boost::bind(&Client::deadline_handler, this, boost::asio::placeholders::error));
    boost::asio::ip::tcp::resolver::query query(request.host, IntToString(request.port));
    resolver_.async_resolve(query,
                            boost::bind(&Client::handle_resolve, this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::iterator));
  }

  void deadline_handler( const boost::system::error_code& err ) {
    if ( err == boost::asio::error::operation_aborted ) {  //cancel async_wait
    }
    if ( req_info_.using_ssl ) {
      ssl_socket_.lowest_layer().close();
    }
    else {
      socket_.close();
    }
  }

  void stop() {
    deadline_.cancel();
  }

  void handle_resolve(const boost::system::error_code& err,
                              boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
    ProgTrace(TRACE5, "Client::handle_resolve");
    if ( !err ) {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      boost::asio::async_connect(req_info_.using_ssl?ssl_socket_.lowest_layer():socket_, endpoint_iterator,
                                 boost::bind(&Client::handle_connect, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::iterator));
    }
    else {
      ProgError( STDLOG, "handle_resolve error: %s", err.message().c_str() );
      stop();
    }
  }

  void handle_connect(const boost::system::error_code& err,
                              boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
  {
    ProgTrace(TRACE5, "Client::handle_connect");
    if ( !err ) {
      if ( req_info_.using_ssl ) {
        ssl_socket_.async_handshake(boost::asio::ssl::stream_base::client,
                                    boost::bind(&Client::handle_handshake, this,
                                                boost::asio::placeholders::error));
      }
      else {
        handle_handshake(err);
      }
    }
    else {
      ProgError( STDLOG, "handle_connect error: %s", err.message().c_str() );
      stop();
    }
  }
  void handle_handshake(const boost::system::error_code& err) {
    ProgTrace(TRACE5, "Client::handle_handshake");
    if ( !err ) {
      ProgTrace(TRACE5, "Client::handle_handshake: Conection established");
      std::ostringstream ns_str;
      if ( req_info_.method.empty() ) {
        req_info_.method = "POST";
      }
      ns_str << req_info_.method + " " << req_info_.path << " HTTP/1.1\r\n";
      ns_str << "Content-Length: " << req_info_.content.length() << "\r\n";
      ns_str << "Host: " << req_info_.host << ":" << req_info_.port << "\r\n";
      ns_str << "Content-Type: text/xml; charset=\"UTF-8\"\r\n";
      if ( !req_info_.action.empty() ) {
        ns_str << "SOAPAction: " << req_info_.action << "\r\n";
      }
      for (std::map<std::string,std::string>::iterator iheader=req_info_.headers.begin(); iheader!=req_info_.headers.end(); iheader++) {
        ns_str << iheader->first + ": " + iheader->second + "\r\n";
      }
      ns_str << "\r\n";
      ns_str << req_info_.content;
      ProgTrace(TRACE5, "%s: %s", __FUNCTION__, ns_str.str().c_str());  //!!!vlad

      request_ = ns_str.str();
      if ( req_info_.using_ssl ) {
        boost::asio::async_write(ssl_socket_, boost::asio::buffer(request_),
                                 boost::bind(&Client::handle_write, this, boost::asio::placeholders::error));
      }
      else {
        boost::asio::async_write(socket_, boost::asio::buffer(request_),
                                 boost::bind(&Client::handle_write, this, boost::asio::placeholders::error));
      }
    }
    else {
      ProgError( STDLOG, "handle_handshake error: %s", err.message().c_str() );
      stop();
    }
  }

  void handle_write(const boost::system::error_code& err) {
    ProgTrace(TRACE5, "Client::handle_write");
    if ( !err ) {
      ProgTrace(TRACE5, "Client::handle_write: async_write successful");
      if ( req_info_.using_ssl ) {
        boost::asio::async_read_until(ssl_socket_, reply_, "\r\n",
                                      boost::bind(&Client::handle_read_status, this,
                                                   boost::asio::placeholders::error,
                                                   boost::asio::placeholders::bytes_transferred));
      }
      else {
        boost::asio::async_read_until(socket_, reply_, "\r\n",
                                      boost::bind(&Client::handle_read_status, this,
                                                   boost::asio::placeholders::error,
                                                   boost::asio::placeholders::bytes_transferred));
      }
    }
    else {
     ProgError( STDLOG, "handle_write error: %s", err.message().c_str() );
     stop();
    }
  }

  void handle_read_status(const boost::system::error_code& err, size_t bytes_transferred) {
    if ( !err ) {
      ProgTrace(TRACE5, "Client::handle_read_status: async_read successful");
      std::istream response_stream(&reply_);
      std::string http_version;
      response_stream >> http_version;
      if ( !response_stream || http_version.empty() || http_version.substr(0, 5) != "HTTP/" ) {
        ProgError( STDLOG, "handle_read_status http_version: %s", http_version.c_str() );
        return;
      }
      response_stream >> res_info_.status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if ( res_info_.status_code != 200 ) {    // 200 = status code OK
        ProgError( STDLOG, "handle_read_status http_version: %s", http_version.c_str() );
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      if ( req_info_.using_ssl ) {
        boost::asio::async_read_until(ssl_socket_, reply_, "\r\n\r\n",
                                      boost::bind(&Client::handle_read_headers, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
      }
      else {
        boost::asio::async_read_until(socket_, reply_, "\r\n\r\n",
                                      boost::bind(&Client::handle_read_headers, this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred));
      }
    }
    else {
      ProgError( STDLOG, "handle_read_status error: %s", err.message().c_str() );
      stop();
    }
  }

  void handle_read_headers(const boost::system::error_code& err, size_t bytes_transferred) {
     ProgTrace(TRACE5, "Client::handle_read_headers: async_read successful");
     if ( !err ) {
       // Process the response headers.
       std::istream response_stream(&reply_);
       std::string header;
       while ( std::getline(response_stream, header) && header != "\r" ) {
         res_info_.headers.push_back( header );
         header = lowerc( header );
         size_t p = header.find( "content-length:" );
         if ( p != std::string::npos ) {
           header = header.substr( p + std::string("content-length:").length() );
           p = header.find( "\r" );
           if ( p != std::string::npos ) {
             header = header.substr( 0, p );
           }
           int len;
           if ( BASIC::StrToInt( header.c_str(), len ) == EOF ) {
             ProgError( STDLOG, "handle_read_headers invalid property Content-length: %s", header.c_str() );
           }
           else {
             res_info_.content_length = len;
             res_info_.pr_contentLength = true;
             ProgTrace( TRACE5, "res_info_.content_length=%d", res_info_.content_length );
           }
         }
         ProgTrace(TRACE5, "handle_read_header: header: %s, reply_.size()=%zu", header.c_str(), reply_.size());
       }
       if ( reply_.size() > 0 ) { // content start!
         std::stringstream ss;
         ss << &reply_;
         res_info_.content += ss.str();
         ProgTrace(TRACE5, "handle_read_header: part of bufer=%s, length=%zu", res_info_.content.c_str(), res_info_.content.length());
       }
       if ( res_info_.content.length() < res_info_.content_length ) {
         if ( req_info_.using_ssl ) {
           tst();
           boost::asio::async_read(ssl_socket_, reply_, boost::asio::transfer_at_least(1),
                                   boost::bind(&Client::handle_read_content, this,
                                   boost::asio::placeholders::error));
         }
         else {
           boost::asio::async_read(socket_, reply_, boost::asio::transfer_at_least(1),
                                   boost::bind(&Client::handle_read_content, this,
                                   boost::asio::placeholders::error));
         }
       }
       else {
         answerReady( err );
       }
       tst();
     }
     else {
       ProgError( STDLOG, "handle_read_headers error: %s", err.message().c_str() );
       stop();
     }
  }

  void answerReady( const boost::system::error_code& err ) {
    if ( err && err != boost::asio::error::eof ) {
        ProgError( STDLOG, "answerReady error: %s", err.message().c_str() );
    }
    else {
      ProgTrace( TRACE5, "handle_read_body end file: %s, res_info_.content_length=%d,  res_info_.content.length()=%zu",
                 err.message().c_str(), res_info_.content_length, res_info_.content.length() );
      if ( res_info_.content_length < res_info_.content.length() ) {
        res_info_.content = res_info_.content.substr( 0, res_info_.content_length - 1 );
      }
      ProgTrace( TRACE5, "handle_read_body end file: %s", res_info_.content.c_str() );
      res_info_.ready = true;
    }
    stop();
  }

  void handle_read_content(const boost::system::error_code& err) {
    if ( !err ) {
      ProgTrace(TRACE5, "Client::handle_read_body: async_read successful");
      std::stringstream ss;
      ss << &reply_;
      res_info_.content += ss.str();
      // Continue reading remaining data until EOF or content_Length = body.size().
      if ( res_info_.content.length() < res_info_.content_length ) {
        if ( req_info_.using_ssl ) {
          boost::asio::async_read(ssl_socket_, reply_, boost::asio::transfer_at_least(1),
                                  boost::bind(&Client::handle_read_content, this,
                                  boost::asio::placeholders::error));
        }
        else {
          boost::asio::async_read(socket_, reply_, boost::asio::transfer_at_least(1),
                                  boost::bind(&Client::handle_read_content, this,
                                  boost::asio::placeholders::error));
        }
      }
      else {
        answerReady( err );
      }
    }
    else {
      ProgError( STDLOG, "handle_read_status error: %s", err.message().c_str() );
      stop();
    }
  }
  const ResponseInfo &response() { return res_info_; }
};


void httpClient_main(const RequestInfo& request, ResponseInfo& response)
{
  try
  {
    boost::asio::io_service io_service;

    Client c(io_service, request);

    io_service.run();
    // Block until the asynchronous operation has completed. c.stop() - stoped client
    //do io_service.run_one(); while (c.would_block());

    response=c.response();
    ProgTrace( TRACE5, "response return %s", response.toString().c_str() );
  }
  catch (std::exception& e)
  {
    throw Exception("httpClient_main: %s",e.what());
  }
}

void send_apis_tr()
{
  TFileQueue file_queue;
  file_queue.get( *TApisTRFilter::Instance() );
  ProgTrace(TRACE5, "send_apis_tr: Num of items in queue: %zu \n", file_queue.size());
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
      request.timeout = 60000;
      TFileQueue::sendFile(item->id);
      ResponseInfo response;
      httpClient_main(request, response);
      process_reply(response.content);
      TFileQueue::doneFile(item->id);
      createMsg( *item, evCommit );
  }
}

void process_reply(const std::string& result)
{
  ProgTrace(TRACE5, "process_reply: %s", result.c_str());
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

