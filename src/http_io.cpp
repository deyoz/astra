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
#include "misc.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace boost::asio;
using namespace pion::net;


void my_test_old()
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

//    pion::net::HTTPRequest post(resource);
    pion::net::HTTPRequest post("/OutBsmService.asmx/BsmProccess?message=string");

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
    }

    pion::net::HTTPResponse rc(post);

    for(int i = 0; i < 2; i++) {

        TPerfTimer tm;
        tm.Init();
        post.send(tcp_conn, error_code);
        if(error_code)
            throw Exception("send failed: %s", error_code.message().c_str());
        ProgTrace(TRACE5,"send succeed");

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
        ProgTrace(TRACE5, "send msg %d: %s", i, tm.PrintWithMessage().c_str());

        if(error_code = tcp_conn.connect(host,port))
            throw Exception("connect failed: %s", error_code.message().c_str());
        ProgTrace(TRACE5,"connect succeed");
    }
}

struct IOHandler {
    string answer;
    HTTPRequestPtr post;
    HTTPRequestWriterPtr writer;
    HTTPResponseReaderPtr reader;
    bool processed;
    void handleWrite(const boost::system::error_code &write_error, std::size_t bytes_written);
    void handleRead(pion::net::HTTPResponsePtr& response, pion::net::TCPConnectionPtr& tcp_conn);
    IOHandler(
            TCPConnectionPtr tcp_conn,
            string host,
            u_int port,
            string resource
            );
};

IOHandler::IOHandler(
        TCPConnectionPtr tcp_conn,
        string host,
        u_int port,
        string resource
        )
{
    processed = false;


    ostringstream host_port;
    host_port << host << ":" << port;

    post = HTTPRequestPtr(new HTTPRequest(resource));
    post->setMethod("GET");
    post->addHeader("Host", host_port.str());

    writer = HTTPRequestWriter::create(tcp_conn, post);
    writer->send(boost::bind(&IOHandler::handleWrite, this, _1, _2));

    reader = HTTPResponseReader::create(tcp_conn, *(post), boost::bind(&IOHandler::handleRead, this, _1, _2));
    //        reader->setTimeout(15);
    reader->receive();
}

void IOHandler::handleWrite(const boost::system::error_code &write_error, std::size_t bytes_written)
{
    if(write_error) // encountered error sending request data
        throw Exception("connect failed: %s", write_error.message().c_str());
    else {
        ProgTrace(TRACE5, "hanldeWrite request: %s", post->getContent());
        ProgTrace(TRACE1, "handleWrite() : %d bytes_written", bytes_written);
    }
}

void IOHandler::handleRead(pion::net::HTTPResponsePtr& response, pion::net::TCPConnectionPtr& tcp_conn)
{
    processed = true;
    //        ProgTrace(TRACE1,"handleRead() : valid: %s",(response->isValid() ? "true" : "false"));
    ProgTrace(TRACE5,"handleRead() : status: %d",response->getStatusCode());
    //ProgTrace(TRACE5,"handleRead() : message: %s",response->getStatusMessage().c_str());
    //        ProgTrace(TRACE5,"handleRead() : content_length: %d",response->getContentLength());

    const char* content_ptr = response->getContent();
    answer = content_ptr ? content_ptr : "";
    ProgTrace(TRACE5, "handleRead() : content_ptr: %s", (content_ptr ? content_ptr : "(nil)"));

    if(response->getStatusCode() != 200 /*and response->getStatusCode() != 500*/) // as the peer is a complete moron
        throw Exception("Failed request: %s", post->getContent());
}

void my_test()
{
    TPerfTimer tm;
    tm.Init();

    vector<string> result;
    string host = "bsm.icfairports.com";
    u_int port = 80;
    string resource = "/OutBsmService.asmx/BsmProccess?message=string";

    io_service io_service;
    TCPConnectionPtr tcp_conn = TCPConnectionPtr(new TCPConnection(io_service, false));
    ProgTrace(TRACE1, "connect(%s, %u)", host.c_str(), port);
    if(boost::system::error_code error_code = tcp_conn->connect(host,port))
        throw Exception("connect failed: %s", error_code.message().c_str());
    ProgTrace(TRACE1,"connected");

    typedef boost::shared_ptr<IOHandler> IOHPtr;
    vector<IOHPtr> ioh_list;
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));
    ioh_list.push_back(IOHPtr(new IOHandler(tcp_conn, host, port, resource)));

    io_service.run();

    for(vector<IOHPtr>::iterator iv = ioh_list.begin(); iv != ioh_list.end(); iv++) {
        if(not (*iv)->processed)
            throw Exception("no answer for request: %s", (*iv)->post->getContent());
        result.push_back((*iv)->answer);
    }

    for(vector<string>::iterator iv = result.begin(); iv != result.end(); iv++)
        ProgTrace(TRACE5, "result: %s", iv->c_str());

    ProgTrace(TRACE5, "send msg: %s", tm.PrintWithMessage().c_str());
}
