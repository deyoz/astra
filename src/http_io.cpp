#include "http_io.h"
#include <boost/asio.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/HTTPRequestWriter.hpp>
#include <pion/net/HTTPResponseReader.hpp>
#include <pion/net/TCPConnection.hpp>

void my_test()
{
    boost::asio::io_service io_service;
    pion::net::TCPConnection tcp_conn(io_service, false);
}

