#include "libra.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "edi_utils.h"
#include "astra_calls.h"
#include "astra_context.h"
#include "httpClient.h"

#include <serverlib/oci8.h>
#include <serverlib/oci8cursor.h>
#include <serverlib/str_utils.h>

#include <optional>

#define NICKNAME "LIBRA"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace OciCpp;

/////////////////////////////////////////////////////////////////////////////////////////

namespace LIBRA {

std::string LIBRA_HTTP_HOST()
{
    static std::string host = readStringFromTcl("LIBRA_HTTP_HOST", "");
    return host;
}

static int LIBRA_HTTP_PORT()
{
    static int port = readIntFromTcl("LIBRA_HTTP_PORT", 0);
    return port;
}

static int LIBRA_HTTP_TIMEOUT()
{
    static int to = readIntFromTcl("LIBRA_HTTP_TIMEOUT", 10);
    return to;
}

static int LIBRA_HTTP_MODE()
{
    static int am = readIntFromTcl("LIBRA_HTTP_MODE", 0);
    return am;
}

//---------------------------------------------------------------------------------------

enum class HttpMode {
    Async = 0,
    Sync  = 1
};

//---------------------------------------------------------------------------------------

class HttpAsyncRequestWasSent {};

//---------------------------------------------------------------------------------------

class LibraHttpClient: public Http::Client
{
protected:
    virtual httpsrv::HostAndPort addr() const
    {
        return httpsrv::HostAndPort(LIBRA_HTTP_HOST(), LIBRA_HTTP_PORT());
    }

    virtual httpsrv::Domain domain() const
    {
        return httpsrv::Domain("LIBRA");
    }

    virtual boost::posix_time::seconds timeout() const
    {
        return boost::posix_time::seconds(LIBRA_HTTP_TIMEOUT());
    }
};

//---------------------------------------------------------------------------------------

const std::string LibraHttpResponse::Status::Ok  = "OK";
const std::string LibraHttpResponse::Status::Err = "ERR";

//---------------------------------------------------------------------------------------

LibraHttpResponse::LibraHttpResponse(const std::string& resp)
    : m_resp(resp)
{
    m_respDoc = ASTRA::createXmlDoc(resp);
}

xmlNodePtr LibraHttpResponse::resultNode() const
{
    if(!m_respDoc) {
        return nullptr;
    }

    return NodeAsNode("/result", m_respDoc->docPtr());
}

boost::optional<LibraHttpResponse> LibraHttpResponse::read()
{
    LibraHttpClient libraClient;
    boost::optional<httpsrv::HttpResp> resp = libraClient.receive();
    if(!resp) {
        LogError(STDLOG) << "Empty Http response from Libra!";
        return {};
    }

    if(resp->commErr) {
        LogError(STDLOG) << "Http communication error! "
                         << "(" << resp->commErr->code << "/" << resp->commErr->errMsg << ")";
        return {};
    }

    const auto fnd = resp->text.find("<result>");
    if(fnd == std::string::npos) {
        LogWarning(STDLOG) << "Invalid Http response from Libra!";
        return {};
    }

    return LibraHttpResponse(resp->text.substr(fnd));
}

//---------------------------------------------------------------------------------------

bool asyncHttpMode()
{
    return LIBRA_HTTP_MODE() == static_cast<int>(HttpMode::Async);
}

//---------------------------------------------------------------------------------------

bool needSendHttpRequest()
{
    // TODO временная реализация - на период отладки:
    // идем по http только для пультов, начинающихся с МОВ..
    std::string desk = TReqInfo::Instance()->desk.code;
    return desk.substr(0, 3) == "МОВ";
}

void synchronousHttpRequest(const std::string& req, const std::string& path)
{
    const std::string reqText = ConvertCodepage(req, "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendRequest(reqText, path);
}

void synchronousHttpRequest(xmlDocPtr reqDoc, const std::string& path)
{
    const std::string reqText = ConvertCodepage(XMLTreeToText(reqDoc), "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendRequest(reqText, path);
}

std::string synchronousHttpExchange(const std::string& req, const std::string& path)
{
    synchronousHttpRequest(req, path);
    return LIBRA::receiveHttpResponse();
}

void asynchronousHttpRequest(const std::string& req, const std::string& path)
{
    int reqCtxtId = AstraContext::SetContext("TERM_REQUEST", req);

    const std::string reqText = ConvertCodepage(req, "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendRequest(reqText, path, AstraEdifact::createKickInfo(reqCtxtId, "libra"));
    throw LIBRA::HttpAsyncRequestWasSent();
}

std::string receiveHttpResponse()
{
    //<result><status>OK</status><answer><root/></answer></result>
    //<result><status>ERR</status><reason>Exception happened</reason></result>
    //<result><status>ERR</status><code>DB-01</code><reason>Exception happened</reason></result>

    auto resp = LibraHttpResponse::read();
    ASSERT(resp);

    xmlNodePtr resultNode = resp->resultNode();

    xmlNodePtr statusNode = findNodeR(resultNode, "status");
    ASSERT(statusNode);

    auto status = getStrFromXml(statusNode);
    LogTrace(TRACE3) << "status: " << status;

    if(status != LibraHttpResponse::Status::Ok) {
        std::string errReason, errCode;

        xmlNodePtr reasonNode = findNodeR(resultNode, "reason");
        if(reasonNode) {
            errReason = getStrFromXml(reasonNode);
        }

        xmlNodePtr codeNode = findNodeR(resultNode, "code");
        if(codeNode) {
            errCode = getStrFromXml(codeNode);
        }

        throw EXCEPTIONS::ExceptionFmt(STDLOG) << "Inner error: " << errReason << "(" << errCode << ")";
    }


    xmlNodePtr answerNode = findNodeR(resultNode, "answer");
    ASSERT(answerNode);

    xmlNodePtr rootNode = findNode(answerNode, "root");
    ASSERT(rootNode);


    // следующие строки нужны только лишь для того,
    // чтобы создать новый xml-документ, содержащий rootNode без его родителя answerNode
    // не придумал ничего лучше..(
    XMLDoc doc = ASTRA::createXmlDoc2("<tmp/>");
    auto tmpNode = NodeAsNode("/tmp", doc.docPtr());
    CopyNode(tmpNode->parent, rootNode);
    RemoveNode(tmpNode);

    return doc.text();
}

}//namespace LIBRA

/////////////////////////////////////////////////////////////////////////////////////////

namespace  {

std::string getRequestName(xmlNodePtr rootNode)
{
    ASSERT(rootNode);
    std::string requestName = "";
    auto name = getprop(rootNode, "name");
    if(!name) {
        LogWarning(STDLOG) << "Xml from client hasn't name property!";
    } else {
        requestName = std::string(name);
    }

    return requestName;
}

std::string getRequestName(int reqCtxtId)
{
    XMLDoc reqCtxt;
    AstraEdifact::getTermRequestCtxt(reqCtxtId, true, __func__, reqCtxt);
    xmlNodePtr rootNode = NodeAsNode("/root", reqCtxt.docPtr());
    return getRequestName(rootNode);
}

void addOprPult2Req(xmlNodePtr rootNode)
{
    NewTextChild(rootNode, "login", TReqInfo::Instance()->user.login);
    NewTextChild(rootNode, "descr", TReqInfo::Instance()->user.descr);
    NewTextChild(rootNode, "pult",  TReqInfo::Instance()->desk.code);
}

void addRights2Req(xmlNodePtr rootNode)
{
    TReqInfo::Instance()->user.access.toXML(NewTextChild(rootNode, "access"));
}

std::string callLibraPkg(const std::string& xml_in)
{
    std::string xml_out;
    Oci8Session os(STDLOG, mainSession());
    Curs8Ctl cur(STDLOG,
"declare\n"
"   CLOB_OUT clob;\n"
"begin\n"
"   delete from WB_REF_ASTRA_TMP;\n"
"   SP_WB_ASTRA_CALLS(:clob_in, CLOB_OUT);\n"
"   insert into WB_REF_ASTRA_TMP(CLOB_OUT) values (CLOB_OUT);\n"
"end;", &os);
    cur.bindClob(":clob_in", xml_in)
       .exec();

    Curs8Ctl selCur(STDLOG,
"select CLOB_OUT from WB_REF_ASTRA_TMP", &os);
    selCur.defClob(xml_out)
          .EXfet();

    LogTrace(TRACE5) << "xml_out=" << xml_out;
    return xml_out;
}

std::string callLibraHttp(const std::string& reqText)
{
    const std::string path = "/call";

    if(LIBRA::asyncHttpMode()) {
        LIBRA::asynchronousHttpRequest(reqText, path);
    } else {
        return LIBRA::synchronousHttpExchange(reqText, path);
    }

    return ""; // can't be here never
}

std::string callLibra(xmlNodePtr reqRootNode)
{
    // Доклейка пульта, оператора и прав доступа
    addOprPult2Req(reqRootNode);
    addRights2Req(reqRootNode);

    std::string xml_in = XMLTreeToText(reqRootNode->doc);
    LogTrace(TRACE6) << __func__ << " xml_in=" << xml_in;

    if(LIBRA::needSendHttpRequest()) {
        return callLibraHttp(xml_in);
    }

    return callLibraPkg(xml_in);
}

std::string callAstra(xmlNodePtr reqRootNode)
{
    std::string xml_in = XMLTreeToText(reqRootNode->doc);
    LogTrace(TRACE6) << __func__ << " xml_in=" << xml_in;

    auto resultXmlDoc = ASTRA::createXmlDoc("<root/>");
    xmlNodePtr resRootNode = NodeAsNode("/root", resultXmlDoc.docPtr());
    bool result = AstraCalls::callByLibra(reqRootNode, resRootNode);

    xmlSetProp(resRootNode, "name", "astra_call");
    xmlSetProp(resRootNode, "result", result ? "ok" : "err");

    std::string xml_out = resultXmlDoc.text();
    LogTrace(TRACE6) << "xml_out=" << xml_out;
    return xml_out;
}

std::string fillErrorData(const std::string& requestName, const std::string& exceptionMsg)
{
    auto errXmlDoc = ASTRA::createXmlDoc("<root/>");
    xmlNodePtr errRootNode = NodeAsNode("/root", errXmlDoc.docPtr());

    xmlSetProp(errRootNode, "result",   "err");
    xmlSetProp(errRootNode, "name",      requestName);
    xmlSetProp(errRootNode, "exception", exceptionMsg);

    return errXmlDoc.text();
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

void LibraInterface::RequestHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    std::string dataFromClient = NodeAsString("data", reqNode);
    auto clientXmlDoc = ASTRA::createXmlDoc(dataFromClient);
    xmlNodePtr reqRootNode = NodeAsNode("/root", clientXmlDoc.docPtr());

    std::string data;
    std::string requestName = getRequestName(reqRootNode);

    try {
        data = requestName == "astra_call" ? callAstra(reqRootNode)
                                           : callLibra(reqRootNode);
    } catch(const LIBRA::HttpAsyncRequestWasSent&) {
        LogTrace(TRACE6) << "Http request was sent. Waiting for answer...";
        return;
    } catch(const ServerFramework::Exception &e) {
        auto exceptionMsg = e.what();
        LogError(STDLOG) << __func__ << " : " << exceptionMsg;
        data = fillErrorData(requestName, exceptionMsg);
    } catch(...) {
        auto exceptionMsg = "unknown error";
        LogError(STDLOG) << __func__ << " : " << exceptionMsg;
        data = fillErrorData(requestName, exceptionMsg);
    }  

    NewTextChild(resNode, "data", data);
}

void LibraInterface::KickHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int reqCtxtId = getIntPropFromXml(reqNode, "req_ctxt_id", 0);
    ASSERT(reqCtxtId);

    std::string data;
    std::string requestName = getRequestName(reqCtxtId);

    try {
        data = LIBRA::receiveHttpResponse();
    } catch(const ServerFramework::Exception &e) {
        auto exceptionMsg = e.what();
        LogError(STDLOG) << __func__ << ":" << exceptionMsg;
        data = fillErrorData(requestName, exceptionMsg);
    } catch(...) {
        auto exceptionMsg = "unknown error";
        LogError(STDLOG) << __func__ << " : " << exceptionMsg;
        data = fillErrorData(requestName, exceptionMsg);
    }

    NewTextChild(resNode, "data", data);
}
