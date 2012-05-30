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
#include "oralib.h"
#include "astra_utils.h"
#include "astra_service.h"
#include "tlg/tlg.h"
#include "serverlib/posthooks.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace boost::asio;
using namespace pion::net;

const string FILE_HTTPGET_TYPE = "HTTPGET";


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
    TCPConnectionPtr tcp_conn;
    string answer;
    HTTPRequestPtr post;
    HTTPRequestWriterPtr writer;
    HTTPResponseReaderPtr reader;
    bool processed;
    void handleWrite(const boost::system::error_code &write_error, std::size_t bytes_written);
    void handleRead(pion::net::HTTPResponsePtr& response, pion::net::TCPConnectionPtr& tcp_conn);
    IOHandler(
            io_service &io_service,
            string host,
            u_int port,
            string resource
            );
};

IOHandler::IOHandler(
        io_service &io_service,
        string host,
        u_int port,
        string resource
        )
{
    processed = false;
    tcp_conn = TCPConnectionPtr(new TCPConnection(io_service, false));

    ProgTrace(TRACE1, "connect(%s, %u)", host.c_str(), port);
    if(boost::system::error_code error_code = tcp_conn->connect(host,port))
        throw Exception("connect failed: %d, %s", error_code.value(), error_code.message().c_str());
    ProgTrace(TRACE1,"connected");

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
    //ProgTrace(TRACE1, "handleRead() : content_ptr: %s", (content_ptr ? content_ptr : "(nil)"));

    if(response->getStatusCode() != 200 /*and response->getStatusCode() != 500*/) // as the peer is a complete moron
        throw Exception("Failed request: %d %s",
                response->getStatusCode(),
                response->getStatusMessage().c_str());
}

void web_replace(string &val, string olds, string news)
{
    size_t idx = val.find(olds);
    while(idx != string::npos) {
        val.replace(idx, olds.size(), news);
        idx = val.find(olds);
    }
}

string web_replace(string val)
{
    web_replace(val, " ", "%20");
    web_replace(val, "&", "%26");
    web_replace(val, "=", "%3D");
    web_replace(val, "\xd\xa", "%0D%0A");
    return val;
}

typedef vector< vector<string> > TBSMList;

void send_bsm(const vector<string> host_list, const TBSMList &bsm_list)
{
    TPerfTimer tm;
    tm.Init();

    static const string br = "%0D%0A";

    vector<string> result;
    u_int port = 80;

    io_service io_service;
    typedef boost::shared_ptr<IOHandler> IOHPtr;
    vector<IOHPtr> ioh_list;
    for(TBSMList::const_iterator iv = bsm_list.begin(); iv != bsm_list.end(); iv++) {
        string bsm;
        for(vector<string>::const_iterator i_bsm = iv->begin(); i_bsm != iv->end(); i_bsm++)
            bsm += web_replace(*i_bsm) + br;
        ioh_list.push_back(IOHPtr(new IOHandler(io_service, host_list[0], port, "/OutBsmService.asmx/BsmProccess?message=" + bsm)));
    }

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

string send_bsm(const string host, const string &bsm)
{
    TPerfTimer tm;
    tm.Init();

    u_int port = 80;

    io_service io_service;
    typedef boost::shared_ptr<IOHandler> IOHPtr;
    vector<IOHPtr> ioh_list;

//!!!    ioh_list.push_back(IOHPtr(new IOHandler(io_service, host, port, "/OutBsmService.asmx/BsmProccess?message=" + web_replace(bsm))));
    ioh_list.push_back(IOHPtr(new IOHandler(io_service, host, port, "/cgi-bin/first.pl?message=" + web_replace(bsm))));

    io_service.run();

    string result;
    for(vector<IOHPtr>::iterator iv = ioh_list.begin(); iv != ioh_list.end(); iv++) {
        if(not (*iv)->processed)
            throw Exception("no answer for request: %s", (*iv)->post->getContent());
        result= (*iv)->answer;
    }

    ProgTrace(TRACE5, "send msg: %s", tm.PrintWithMessage().c_str());
    return result;
}

void my_test()
{
    vector<string> host_list;
    //    host_list.push_back("bsm.icfairports.com");
    host_list.push_back("bsm.icfairports.com");
    host_list.push_back("bsm2.icfairports.com");
    TBSMList bsm_list;


    TQuery Qry(&OraSession);
    Qry.SQLText = "select body from tlg_out where type = 'BSM' and point_id = :point_id";
    Qry.CreateVariable("point_id", otInteger, 2042638);
    Qry.Execute();
    static const string br = "\xd\xa";
    for(; not Qry.Eof; Qry.Next()) {
        vector<string> bsm;
        string body = Qry.FieldAsString("body");
        size_t idx = body.find(br);
        while(idx != string::npos) {
            bsm.push_back(body.substr(0, idx));
            body.erase(0, idx + br.size());
            idx = body.find(br);
        }
        bsm_list.push_back(bsm);
    }

    for(TBSMList::iterator iv = bsm_list.begin(); iv != bsm_list.end(); iv++) {
        for(vector<string>::iterator i_bsm = iv->begin(); i_bsm != iv->end(); i_bsm++)
            ProgTrace(TRACE5, "<%s>", i_bsm->c_str());
    }

    send_bsm(host_list, bsm_list);
}

void http_send_zaglushka(vector<string> &bsm_bodies)
{
    ProgTrace(TRACE5, "http_send_zaglushka");
    map<string, string> fileparams;

    fileparams["ADDR1_HTTP"] = "astrabet.komtex";
    fileparams["ADDR2_HTTP"] = "astrabeta1.komtex";
    fileparams["ADDR3_HTTP"] = "astrabeta2.komtex";
    fileparams["ADDR4_HTTP"] = "astrabeta3.komtex";
    fileparams["ADDR5_HTTP"] = "astrabeta4.komtex";
    fileparams["ADDR6_HTTP"] = "astrabeta5.komtex";
    fileparams["ADDR7_SITA"] = "astrabeta2.komtex";

    for(vector<string>::iterator iv = bsm_bodies.begin(); iv != bsm_bodies.end(); iv++) {
        putFile( OWN_POINT_ADDR(),
                OWN_POINT_ADDR(),
                FILE_HTTPGET_TYPE,
                fileparams,
                *iv );
    }
    registerHookAfter(sendCmdTlgHttpSnd);
}

