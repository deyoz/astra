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

static int LIBRA_HTTP_TRYCOUNT()
{
    static int tc = readIntFromTcl("LIBRA_HTTP_TRYCOUNT", 1);
    return tc;
}

static int LIBRA_HTTP_MODE()
{
    static int am = readIntFromTcl("LIBRA_HTTP_MODE", 0);
    return am;
}

static bool LIBRA_HTTP_STAT()
{
    static int stat = readIntFromTcl("LIBRA_HTTP_STAT", 0);
    return static_cast<bool>(stat);
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

    virtual unsigned maxTryCount() const
    {
        int tryCount = LIBRA_HTTP_TRYCOUNT();
        ASSERT(tryCount > 0);
        return static_cast<unsigned>(tryCount);
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
    if(LIBRA_HTTP_STAT()) {
        if(auto stat = httpsrv::GetStat()) {
            LogTrace(TRACE1) << stat.get();
        }
    }
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

    const std::string respText = ConvertCodepage(resp->text, "UTF-8", "CP866");
    const auto fnd = respText.find("<result>");
    if(fnd == std::string::npos) {
        LogWarning(STDLOG) << "Invalid Http response from Libra!";
        return {};
    }

    return LibraHttpResponse(respText.substr(fnd));
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

void synchronousHttpGetRequest(const std::string& req, const std::string& path)
{
    const std::string reqText = ConvertCodepage(req, "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendGetRequest(reqText, path);
}

void synchronousHttpPostRequest(const std::string& req, const std::string& path)
{
    const std::string reqText = ConvertCodepage(req, "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendPostRequest(reqText, path);
}

void synchronousHttpGetRequest(xmlDocPtr reqDoc, const std::string& path)
{
    const std::string reqText = ConvertCodepage(XMLTreeToText(reqDoc), "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendGetRequest(reqText, path);
}

void synchronousHttpPostRequest(xmlDocPtr reqDoc, const std::string& path)
{
    const std::string reqText = ConvertCodepage(XMLTreeToText(reqDoc), "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendPostRequest(reqText, path);
}

std::string synchronousHttpGetExchange(const std::string& req, const std::string& path)
{
    synchronousHttpGetRequest(req, path);
    return LIBRA::receiveHttpResponse();
}

std::string synchronousHttpPostExchange(const std::string& req, const std::string& path)
{
    synchronousHttpPostRequest(req, path);
    return LIBRA::receiveHttpResponse();
}

void asynchronousHttpGetRequest(const std::string& req, const std::string& path)
{
    int reqCtxtId = AstraContext::SetContext("TERM_REQUEST", req);

    const std::string reqText = ConvertCodepage(req, "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendGetRequest(reqText, path, AstraEdifact::createKickInfo(reqCtxtId, "libra"));
    throw LIBRA::HttpAsyncRequestWasSent();
}

void asynchronousHttpPostRequest(const std::string& req, const std::string& path)
{
    int reqCtxtId = AstraContext::SetContext("TERM_REQUEST", req);

    const std::string reqText = ConvertCodepage(req, "CP866", "UTF-8");

    LIBRA::LibraHttpClient libraClient;
    libraClient.sendPostRequest(reqText, path, AstraEdifact::createKickInfo(reqCtxtId, "libra"));
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
    XMLDoc doc = ASTRA::createXmlDoc("<tmp/>");
    auto tmpNode = NodeAsNode("/tmp", doc.docPtr());
    CopyNode(tmpNode->parent, rootNode);
    RemoveNode(tmpNode);

    return doc.text();
}

FieldData::Type FieldData::getDataType(const std::string& date_type)
{
    if (date_type == "int") {
        return Type::Integer;
    }
    if (date_type == "date") {
        return Type::DateTime;
    }
    return Type::String;
}

bool FieldData::getIsNull(const std::string& is_null)
{
    return is_null == "true";
}

int FieldData::fieldAsInteger() const
{
    if (data_type != Type::Integer) {
        throw EXMLError("Field is not integer");
    }
    if (is_null) {
      return 0;
    }
    return std::stoi(value);
}

const std::string& FieldData::fieldAsString() const
{
    if (data_type != Type::String) {
        throw EXMLError("Field is not string");
    }
    return value;
}

BASIC::date_time::TDateTime FieldData::fieldAsDateTime() const
{
    if (data_type != Type::DateTime) {
        throw EXMLError("Field is not DateTime");
    }
    BASIC::date_time::TDateTime result;
    if (BASIC::date_time::StrToDateTime(value.c_str(), date_format.c_str(), result) == EOF) {
        throw EXMLError("Cannot convert field to DateTime, format=" + date_format);
    }
    return result;
}

std::string makeHttpQueryString(const std::map<std::string,std::string>& dict)
{
    std::ostringstream result;
    bool first = true;
    for (const auto& item: dict) {
        const std::string key = StrUtils::url_encode(item.first);
        const std::string value = StrUtils::url_encode(item.second);
        if (first) {
            result << "?";
            first = false;
        } else {
            result << "&";
        }
        result << key << "=" << value;
    }
    return result.str();
}

std::vector<RowData> getHttpRequestDataRows(const std::string& request,
                                            const std::string& params)
{
    LogTrace(TRACE6) << __func__
                    << ": request='" << request << "'"
                    << ", params='" << params << "'";
    std::vector<RowData> result;
    const std::string answer = synchronousHttpGetExchange("", request + params);
    LogTrace(TRACE6) << __func__
                     << ": answer='" << answer << "'";
    if (answer.empty()) {
        throw EXMLError(std::string(__func__) + ": Empty answer");
    }
    XMLDoc data = ASTRA::createXmlDoc(answer);
    xmlNodePtr root = findNodeR(data.docPtr()->children, "root");
    if (root == NULL) {
        throw EXMLError(std::string(__func__) + ": Root node not found");
    }

    int row_count = 0;
    for (xmlNodePtr row_node = root->children; row_node != NULL;
         row_node = row_node->next, row_count++)
    {
        const int row_number = getIntPropFromXml(row_node, "row_number");
        RowData row;
        int col_count = 0;
        for (xmlNodePtr col_node = row_node->children; col_node != NULL;
             col_node = col_node->next, col_count++)
        {
            const FieldName_t field_name = reinterpret_cast<const char*>(col_node->name);
            const bool success = row.emplace(field_name,
                                             FieldData(row_number, col_node)).second;
            if (!success) {
                throw EXMLError("Duplicate field " + field_name);
            }
        }
        result.push_back(row);
    }
    return result;
}

RowData getHttpRequestDataRow(const std::string& request, const std::string& params)
{
    const std::vector<RowData> rows = getHttpRequestDataRows(request, params);
    if (rows.empty()) {
        throw EXMLError(std::string(__func__) + ": Data not found");
    }
    if (rows.size() != 1) {
        throw EXMLError(std::string(__func__) + ": More than one row");
    }
    return rows.front();
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

void accessRightstoXML(const TAccess& access, xmlNodePtr accessNode)
{
    if(accessNode==NULL) return;

    //права доступа к операциям
    xmlNodePtr rightsNode = NewTextChild(accessNode, "rights");
    const std::set<int> wbAccessRights = { 1010, 1011, 1012, 1013 };
    for(auto right: access.rights().elems()) {
        if(algo::contains(wbAccessRights, right)) {
            NewTextChild(rightsNode, "right", right);
        }
    }

    //права доступа к авиакомпаниям
    xmlNodePtr airlinesNode = NewTextChild(accessNode, "airlines");
    for(auto airline: access.airlines().elems()) {
        NewTextChild(airlinesNode, "airline", airline);
    }
    NewTextChild(accessNode, "airlines_permit", static_cast<int>(access.airlines().elems_permit()));

    //права доступа к аэропортам
    xmlNodePtr airpsNode = NewTextChild(accessNode, "airps");
    for(auto airp: access.airps().elems()) {
        NewTextChild(airpsNode, "airp", airp);
    }
    NewTextChild(accessNode, "airps_permit", static_cast<int>(access.airps().elems_permit()));
}

void addRights2Req(xmlNodePtr rootNode)
{
    xmlNodePtr accessNode = NewTextChild(rootNode, "access");
    accessRightstoXML(TReqInfo::Instance()->user.access, accessNode);
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
    cur.throwError(NO_DATA_FOUND)
       .bindClob(":clob_in", xml_in)
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
        LIBRA::asynchronousHttpPostRequest(reqText, path);
    } else {
        return LIBRA::synchronousHttpPostExchange(reqText, path);
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
