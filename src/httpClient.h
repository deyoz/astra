#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include "file_queue.h"
#include "astra_utils.h"

const std::string APIS_TR = "APIS_TR";

struct RequestInfo
{
  std::string host;
  uint16_t port;
  std::string path;
  std::string action;
  std::string content;
  std::string login;
  std::string pswd;
  bool using_ssl;
};

class TApisTRFilter {
    private:
    public:
        static TFilterQueue *Instance() {
            static TFilterQueue *_instance = 0;
            if ( !_instance ) {
                _instance = new TFilterQueue( OWN_POINT_ADDR(), APIS_TR, ASTRA::NoExists, ASTRA::NoExists, true, 10 );
            }
            return _instance;
        }
};


class Client
{
public:
  Client(boost::asio::io_service& io_service, boost::asio::ssl::context& context,
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator, RequestInfo& request)
    : req_info_(request), ssl_socket_(io_service, context), socket_(io_service)
  {
    boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
    if (req_info_.using_ssl) {
      ssl_socket_.lowest_layer().async_connect(endpoint,
          boost::bind(&Client::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
    }
    else {
      (void) context;
      socket_.lowest_layer().async_connect(endpoint,
                boost::bind(&Client::handle_connect, this,
                  boost::asio::placeholders::error, ++endpoint_iterator));
    }
  }

  void handle_connect(const boost::system::error_code& error,
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

  void handle_handshake(const boost::system::error_code& error);

  void handle_write(const boost::system::error_code& error);

  void handle_read_header(const boost::system::error_code& error, size_t bytes_transferred);

  void handle_read_body(const boost::system::error_code& error);

private:
  RequestInfo req_info_;
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_;
  boost::asio::ip::tcp::socket socket_;
  std::string request_;
  boost::asio::streambuf reply_;
};

int httpClient_main(RequestInfo& request);
void send_apis_tr();
void process_reply (const std::string& result);

#endif // HTTPCLIENT_H
