#include "httpClient.h"
#include "file_queue.h"
#include "astra_utils.h"
#include "edi_utils.h"
#include "misc.h"
#include "tlg/request_params.h"

#include <libtlg/tlg_outbox.h>
#include <serverlib/str_utils.h>
#include <serverlib/testmode.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/query_runner.h>
#include <serverlib/EdiHelpManager.h>

#include <fstream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdlib.h>

#define NICKNAME "ANNA"
#include <serverlib/slogger.h>
#include "exceptions.h"

using namespace EXCEPTIONS;

std::string StrHTTPErrorOperation( const THTTPErrorOperation &operation ) {
  std::string res;
  switch ( operation ) {
    case toDeadline:
      res = "Deadline";
      break;
    case toResolve:
      res = "Resolve";
      break;
    case toConnect:
      res = "Connect";
      break;
    case toHandshake:
      res = "Handshake";
      break;
    case toWrite:
      res = "Write";
      break;
    case toStatus:
      res = "Status";
      break;
    case toReadHeaders:
      res = "ReadHeaders";
      break;
    case toAnswerReady:
      res = "AnswerReady";
      break;
    case toReadContent:
      res = "ReadContent";
      break;
    default:
      res = "Unknown";
  }
  return res;
};


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

public:
  Client(boost::asio::io_service& io_service, const RequestInfo& request)
    : req_info_(request), deadline_(io_service),  resolver_(io_service),
      ctx(io_service, boost::asio::ssl::context::sslv23_client),
      ssl_socket_(io_service, ctx), socket_(io_service) {
    //  would_block_(true) {

    ctx.set_verify_mode(boost::asio::ssl::context::verify_none); //??? �������� ���� ������ ��। ��������஬ ssl_socket_???
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
    if ( err == boost::asio::error::operation_aborted ) {  //cancel async_wait - call stop
      tst();
    }
    else {
      res_info_.error_code = err.value();
      res_info_.error_operation = toDeadline;
      res_info_.error_message = err.message();
    }
    if ( req_info_.using_ssl ) {
      ssl_socket_.lowest_layer().close();
    }
    else {
      socket_.close();
    }
  }

  void stop( const boost::system::error_code& err, const THTTPErrorOperation &operation_error ) {
    if ( res_info_.error_operation != toDeadline ) {
      res_info_.error_code = err.value();
      res_info_.error_operation = operation_error;
      res_info_.error_message = err.message();
    }
    deadline_.cancel();
  }

  void handle_resolve(const boost::system::error_code& err,
                              boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
    ProgTrace(TRACE5, "Client::handle_resolve");
    if ( !err ) {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      if ( req_info_.using_ssl ) {
        ssl_socket_.lowest_layer().async_connect(endpoint,
                                                 boost::bind(&Client::handle_connect, this,
                                                 boost::asio::placeholders::error, ++endpoint_iterator));
      }
      else {
        socket_.lowest_layer().async_connect(endpoint,
                                             boost::bind(&Client::handle_connect, this,
                                             boost::asio::placeholders::error, ++endpoint_iterator));
      }
    }
    else {
      ProgError( STDLOG, "handle_resolve error: %s", err.message().c_str() );
      stop( err, toResolve );
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
      if ( endpoint_iterator != boost::asio::ip::tcp::resolver::iterator() ) {
        boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
        if ( req_info_.using_ssl ) {
          ssl_socket_.lowest_layer().close();
          ssl_socket_.lowest_layer().async_connect(endpoint,
                                                   boost::bind(&Client::handle_connect, this,
                                                   boost::asio::placeholders::error, ++endpoint_iterator));
        }
        else {
          socket_.lowest_layer().close();
          socket_.lowest_layer().async_connect(endpoint,
                                               boost::bind(&Client::handle_connect, this,
                                               boost::asio::placeholders::error, ++endpoint_iterator));
        }
      }
      else {
        ProgError( STDLOG, "handle_connect error: %s", err.message().c_str() );
        stop( err, toConnect );
      }
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
      if ( !req_info_.login.empty() && !req_info_.pswd.empty() ) {
        ns_str << "Authorization: Basic " << StrUtils::b64_encode(req_info_.login + ":" + req_info_.pswd) << "\r\n";
      }
      for (std::map<std::string,std::string>::iterator iheader=req_info_.headers.begin(); iheader!=req_info_.headers.end(); iheader++) {
        ns_str << iheader->first + ": " + iheader->second + "\r\n";
      }
      ns_str << "\r\n";
      ns_str << req_info_.content;

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
      stop( err, toHandshake );
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
     stop( err, toWrite );
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
        ProgError( STDLOG, "handle_read_status status_message: %s", status_message.c_str() );
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
      ProgError( STDLOG, "handle_read_status error: %d,%s", err.value(), err.message().c_str() );
      stop( err, toStatus );
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
           if ( StrToInt( header.c_str(), len ) == EOF ) {
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
         //ProgTrace(TRACE5, "handle_read_header: part of bufer=%s, length=%zu", res_info_.content.c_str(), res_info_.content.length());
       }
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
         answerReady( err, toReadHeaders );
       }
     }
     else {
       ProgError( STDLOG, "handle_read_headers error: %s", err.message().c_str() );
       stop( err, toReadHeaders );
     }
  }

  void answerReady( const boost::system::error_code& err, const THTTPErrorOperation &operation_error ) {
    if ( err && err != boost::asio::error::eof ) {
      ProgError( STDLOG, "answerReady error: %s", err.message().c_str() );
    }
    else {
      ProgTrace( TRACE5, "handle_read_body end file: %s, res_info_.content_length=%d,  res_info_.content.length()=%zu",
                 err.message().c_str(), res_info_.content_length, res_info_.content.length() );
      if ( res_info_.content_length < res_info_.content.length() ) {
        res_info_.content = res_info_.content.substr( 0, res_info_.content_length - 1 );
      }
      //ProgTrace( TRACE5, "handle_read_body end file: %s", res_info_.content.c_str() );
      res_info_.completed = true;
    }
    stop( err, operation_error );
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
        answerReady( err, toReadContent );
      }
    }
    else {
      ProgError( STDLOG, "handle_read_status error: %s", err.message().c_str() );
      stop( err, toReadContent );
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
    if ( !response.completed ) {
      ProgError( STDLOG, "httpClient_main: response return %s", response.toString().c_str() );
    }
    else {
      ProgTrace( TRACE5, "response return %s", response.toString().c_str() );
    }
  }
  catch (std::exception& e)
  {
    throw Exception("httpClient_main: %s",e.what());
  }
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace Http {

static std::string makeHttpPostRequest(const std::string& resource,
                                       const std::string& host,
                                       const std::string& postbody)
{
    return "POST " + resource + " HTTP/1.1\r\n"
             "Host: " + host + "\r\n"
             "Content-Type: application/xml; charset=utf-8\r\n"
             "Content-Length: " + HelpCpp::string_cast(postbody.size()) + "\r\n"
             "\r\n" +
             postbody;
}

//---------------------------------------------------------------------------------------

void Client::sendRequest(const std::string& reqText, const std::string& reqPath,
                         const edifact::KickInfo& kickInfo) const
{
    sendRequest_(reqText, reqPath, kickInfo);
}

void Client::sendRequest(const std::string& reqText, const std::string& reqPath) const
{
    sendRequest_(reqText, reqPath, {});
}

void Client::sendRequest_(const std::string& reqText, const std::string& reqPath,
                          const boost::optional<edifact::KickInfo>& kickInfo) const
{
    const std::string httpPost = makeHttpPostRequest(reqPath, addr().host, reqText);

    LogTrace(TRACE5) << "HTTP Request, text:\n" << reqText;

    httpsrv::DoHttpRequest req(ServerFramework::getQueryRunner().getEdiHelpManager().msgId(),
                               domain(), addr(), httpPost);
    req.setTimeout(timeout())
       .setMaxTryCount(1)
       .setSSL(useSsl());

    if(kickInfo) {
        req.setPeresprosReq(AstraEdifact::make_xml_kick(*kickInfo))
           .setDeffered(true);
    }

#ifdef XP_TESTING
    if (inTestMode()) {
        const std::string httpPostCP866 = UTF8toCP866(httpPost);
        LogTrace(TRACE1) << "request: " << httpPostCP866;
        xp_testing::TlgOutbox::getInstance().push(tlgnum_t("httpreq"),
                        StrUtils::replaceSubstrCopy(httpPostCP866, "\r", ""), 0 /* h2h */);
    }
#endif // XP_TESTING

    req();
}

boost::optional<httpsrv::HttpResp> Client::receive() const
{
    const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(
                ServerFramework::getQueryRunner().getEdiHelpManager().msgId(),
                domain());

    for (const httpsrv::HttpResp& httpResp: responses)
    {
        LogTrace(TRACE1) << "httpResp text: '" << httpResp.text << "'";
    }

    const size_t responseCount = responses.size();
    if (responseCount == 0)
    {
        LogTrace(TRACE1) << "FetchHttpResponses: hasn't got any responses yet";
        return {};
    }
    if (responseCount > 1)
    {
        LogError(STDLOG) << "FetchHttpResponses: " << responseCount << " responses!";
    }

    return responses.front();
}

httpsrv::UseSSLFlag Client::useSsl() const
{
    return httpsrv::UseSSLFlag(false);
}

}//namespace Http
