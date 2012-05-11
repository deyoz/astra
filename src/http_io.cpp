#include "http_io.h"
#include <boost/asio.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponse.hpp>
#include <pion/net/HTTPRequestWriter.hpp>
#include <pion/net/HTTPResponseReader.hpp>
#include <pion/net/TCPConnection.hpp>

#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include "exceptions.h"

using namespace std;
using namespace EXCEPTIONS;

void my_test()
{
    boost::asio::io_service io_service;
    pion::net::TCPConnection tcp_conn(io_service, false);

    string host = "bsm.icfairports.com";
    u_int port = 80;
    string resource = "/OutBsmService.asmx/";
    string action = "BsmProccess";
    string content =
        "<?xml version='1.0' encoding='utf-8'?>"
        "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
        "  <soap:Body>"
        "    <BsmProccess xmlns=\"http://tempuri.org/\">"
        "      <message>string</message>"
        "    </BsmProccess>"
        "  </soap:Body>"
        "</soap:Envelope>";

    ostringstream host_port;
    host_port << host << ":" << port;

    ProgTrace(TRACE5,"connect to %s:%d", host.c_str(), port);
    boost::system::error_code error_code;
    if(error_code = tcp_conn.connect(host,port))
        throw Exception("connect failed: %s", error_code.message().c_str());
    ProgTrace(TRACE5,"connect succeed");

    pion::net::HTTPRequest post(resource);

    if(false) {
        // POST
        post.setMethod("POST");
        post.addHeader("SOAPAction", action);
        post.addHeader("Host", host_port.str());
        post.setContentType("text/xml; charset=utf-8");
        post.setContent(content);
    } else {
        // GET
        post.setMethod("GET");
        post.addHeader("Host", host_port.str());
        post.setResource("/OutBsmService.asmx/BsmProccess?message=string");
    }

    post.send(tcp_conn, error_code);
    if(error_code)
        throw Exception("send failed: %s", error_code.message().c_str());
    ProgTrace(TRACE5,"send succeed");

    pion::net::HTTPResponse rc(post);
    rc.receive(tcp_conn, error_code);
    if(error_code)
        throw Exception("receive failed: %s", error_code.message().c_str());

    if(rc.getStatusCode() != 200)
    {
        ProgTrace(TRACE5, "Status(%i :: %s)", rc.getStatusCode(), rc.getStatusMessage().c_str());
        ProgTrace(TRACE5, "    |%s|", rc.getContent());
        throw Exception(rc.getContent());
    }

    const char* content_ptr = rc.getContent();
    string result = content_ptr ? std::string(content_ptr, rc.getContentLength()) : "";
    ProgTrace(TRACE5, "response: %s", result.c_str());
}

