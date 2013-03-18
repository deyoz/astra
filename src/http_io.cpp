#include "http_io.h"
#include <boost/asio.hpp>
#include <pion/http/request.hpp>
#include <pion/http/response.hpp>
#include <pion/http/request_writer.hpp>
#include <pion/http/response_reader.hpp>
#include <pion/tcp/connection.hpp>

#define NICKNAME "DENIS"
#include "serverlib/test.h"
#include "exceptions.h"
#include "misc.h"
#include "oralib.h"
#include "astra_utils.h"
#include "astra_service.h"
#include "tlg/tlg.h"
#include "serverlib/posthooks.h"
#include "xml_unit.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace boost::asio;
using namespace pion::http;



void my_test_old()
{
    boost::asio::io_service io_service;
    pion::tcp::connection tcp_conn(io_service, false);

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
    pion::http::request post("/OutBsmService.asmx/BsmProccess?message=string");

    if(false) {
        // POST
        post.set_method("POST");
        post.add_header("SOAPAction", action);
        post.add_header("Host", host_port.str());
        post.set_content_type("text/xml; charset=utf-8");
        post.set_content(content);
    } else {
        // GET
        post.set_method("GET");
        post.add_header("Host", host_port.str());
    }

    pion::http::response rc(post);

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

        if(rc.get_status_code() != 200)
        {
            ProgTrace(TRACE5, "Status(%i :: %s)", rc.get_status_code(), rc.get_status_message().c_str());
            ProgTrace(TRACE5, "    |%s|", rc.get_content());
            throw Exception(rc.get_content());
        }

        const char* content_ptr = rc.get_content();
        string result = content_ptr ? std::string(content_ptr, rc.get_content_length()) : "";
        ProgTrace(TRACE5, "response: %s", result.c_str());
        ProgTrace(TRACE5, "send msg %d: %s", i, tm.PrintWithMessage().c_str());

        if(error_code = tcp_conn.connect(host,port))
            throw Exception("connect failed: %s", error_code.message().c_str());
        ProgTrace(TRACE5,"connect succeed");
    }
}

struct IOHandler {
    pion::tcp::connection_ptr tcp_conn;
    string answer;
    request_ptr post;
    request_writer_ptr writer;
    response_reader_ptr reader;
    bool processed;
    void handleWrite(const boost::system::error_code &write_error, std::size_t bytes_written);
    void handleRead(response_ptr& response, pion::tcp::connection_ptr& tcp_conn);
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
    tcp_conn = pion::tcp::connection_ptr(new pion::tcp::connection(io_service, false));

    if(boost::system::error_code error_code = tcp_conn->connect(host,port))
        throw Exception("connect failed: %d, %s", error_code.value(), error_code.message().c_str());

    ostringstream host_port;
    host_port << host << ":" << port;

    post = request_ptr(new request(resource));
    post->set_method("GET");
    post->add_header("Host", host_port.str());

    writer = request_writer::create(tcp_conn, post);
    writer->send(boost::bind(&IOHandler::handleWrite, this, _1, _2));

    reader = response_reader::create(tcp_conn, *(post), boost::bind(&IOHandler::handleRead, this, _1, _2));
//    reader->setTimeout(0);
    reader->receive();
}

void IOHandler::handleWrite(const boost::system::error_code &write_error, std::size_t bytes_written)
{
    if(write_error) // encountered error sending request data
        throw Exception("connect failed: %s", write_error.message().c_str());
}

void IOHandler::handleRead(response_ptr& response, pion::tcp::connection_ptr& tcp_conn)
{
    processed = true;

    const char* content_ptr = response->get_content();
    answer = content_ptr ? content_ptr : "";

    if(response->get_status_code() != 200 /*and response->getStatusCode() != 500*/) // as the peer is a complete moron
        throw Exception("Failed request: %d %s",
                response->get_status_code(),
                response->get_status_message().c_str());
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
            throw Exception("no answer for request: %s", (*iv)->post->get_content());
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
    ioh_list.push_back(IOHPtr(new IOHandler(io_service, host, port, "/OutBsmService.asmx/BsmProccess?message=" + web_replace(bsm))));

    io_service.run();

    string result;
    for(vector<IOHPtr>::iterator iv = ioh_list.begin(); iv != ioh_list.end(); iv++) {
        if(not (*iv)->processed)
            throw Exception("no answer for request: %s", (*iv)->post->get_query_string().c_str());
        result= (*iv)->answer;
    }

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

void sirena_rozysk_send(const std::string &content)
{
    string host = "uat2.vtsft.ru";
    u_int port = 80;
    string resource = "/ss-services/SirenaSearchService";
    string action = "importASTDate";

    ostringstream host_port;
    host_port << host << ":" << port;

    io_service io_service;
    pion::tcp::connection tcp_conn(io_service, false);

    if(boost::system::error_code error_code = tcp_conn.connect(host,port))
        throw Exception("connect failed: %s", error_code.message().c_str());

    request post(resource);
    post.set_method("POST");
    post.add_header("SOAPAction", action);
    post.add_header("Host", host_port.str());
    post.set_content_type("text/xml;charset=UTF-8");
    post.set_content(content);

    boost::system::error_code failed;
    post.send(tcp_conn, failed);
    if(failed)
        throw Exception("send failed: %s", failed.message().c_str());

    response rc(post);
    rc.receive(tcp_conn, failed);
    if(failed)
        throw Exception("receive failed: %s", failed.message().c_str());

    if(rc.get_status_code() != 200)
        throw Exception("HTTP status not ok: Status(%i :: %s), content: |%s|", rc.get_status_code(), rc.get_status_message().c_str(), rc.get_content());

    const char* content_ptr = rc.get_content();

    string result = (content_ptr ? std::string(content_ptr, rc.get_content_length()) : "");
    if(not result.empty()) {
        xmlDocPtr doc = NULL;
        try {
            doc = TextToXMLTree(result);
        } catch(...) {
        }
        if(doc != NULL) {
            try {
                xmlNodePtr rootNode=xmlDocGetRootElement(doc);
                xmlNodePtr node = rootNode->children;
                node = NodeAsNodeFast("Body", node);
                if(node) {
                    node = node->children;
                    node = NodeAsNodeFast("importASTDateResult", node);
                    if(node) {
                        node = node->children;
                        node = NodeAsNodeFast("operationStatus", node);
                        string status = (node ? NodeAsString(node) : "");
                        if(status != "OK") {
                            ProgTrace(TRACE5, "%s", GetXMLDocText(doc->doc).c_str());
                            throw Exception("Return status not OK: '%s'", status.c_str());
                        }
                    }
                }
            } catch(...) {
                xmlFreeDoc(doc);
                throw;
            }
            xmlFreeDoc(doc);
        } else
            throw Exception("wrong answer XML");
    } else
        throw Exception("result is empty");
}
