#include "cuws.h"
#include "xml_unit.h"
#include "qrys.h"
#include "astra_locale_adv.h"
#include "http_consts.h"
#include "serverlib/str_utils.h"
#include <boost/regex.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;

void dump_content(xmlNodePtr contentNode)
{
    if(not contentNode) return;
    LogTrace(TRACE5) << "content node: " << contentNode->name;
    xmlNodePtr curNode = contentNode->children;
    while(curNode) {
        dump_content(curNode);
        curNode = curNode->next;
    }
}

void to_content(xmlNodePtr resNode, const string &data)
{
    SetProp( NewTextChild(resNode, "content",  data), "b64", true);
}

string getResource(string file_path)
{
    try
    {
        TCachedQuery Qry1(
            "select text from HTML_PAGES, HTML_PAGES_TEXT "
            "where "
            "   HTML_PAGES.name = :name and "
            "   HTML_PAGES.id = HTML_PAGES_TEXT.id "
            "order by "
            "   page_no",
            QParams()
            << QParam("name", otString, file_path)
        );
        Qry1.get().Execute();
        string result;
        for (; not Qry1.get().Eof; Qry1.get().Next())
            result += Qry1.get().FieldAsString("text");
        return result;
    }
    catch(Exception &E)
    {
        cout << __FUNCTION__ << " " << E.what() << endl;
        return "";
    }
}

void CUWSSuccess(xmlNodePtr resNode)
{
    to_content(resNode, getResource("/cuws_success.xml"));
}

void CUWSInternalServerError(xmlNodePtr resNode)
{
    to_content(resNode, getResource("/cuws_error.xml"));
}

void params_from_xml(xmlNodePtr reqNode, const string &name, map<string, string> &params)
{
    xmlNodePtr paramsNode = NodeAsNode(name.c_str(), reqNode);
    if(not paramsNode) throw Exception(name + " node not found");
    xmlNodePtr currNode = paramsNode->children;
    while(currNode) {
        params[NodeAsString("name", currNode)] = NodeAsString("value", currNode);
        currNode = currNode->next;
    }
    for(const auto i: params)
        LogTrace(TRACE5) << name << "[" << i.first << "] = '" << i.second << "'";
}

void CUWSwsdl(xmlNodePtr resNode, const string &uri, const string &host, const string &client_id)
{
    if(host.empty())
        throw Exception("host is empty");
    if(client_id.empty())
        throw Exception("client_id is empty");
    if(uri.empty())
        throw Exception("uri is empty");
    to_content(resNode,
            StrUtils::b64_encode(
                boost::regex_replace(
                    StrUtils::b64_decode(getResource("/cuws.wsdl")),
                    boost::regex("ENDPOINT"),
                    "http://" + host + uri + "?" + AstraHTTP::CLIENT_ID + "=" + client_id)));
}

void CUWSInterface::CUWS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    try {
        map<string, string> header;
        map<string, string> get_params;
        params_from_xml(reqNode, "header", header);
        params_from_xml(reqNode, "get_params", get_params);

        if(get_params.find("wsdl") != get_params.end()) {
            LogTrace(TRACE5) << "wsdl branch";
            CUWSwsdl(
                    resNode,
                    NodeAsString(AstraHTTP::URI_PATH.c_str(), reqNode),
                    header[AstraHTTP::HOST],
                    get_params[AstraHTTP::CLIENT_ID]);
        } else {
            LogTrace(TRACE5) << "wsdl branch";
            CUWSSuccess(resNode);
        }
    } catch(Exception &E) {
        ProgError(STDLOG, "%s: %s", __FUNCTION__, E.what());
        CUWSInternalServerError(resNode);
    } catch(...) {
        ProgError(STDLOG, "%s: unknown error", __FUNCTION__);
        CUWSInternalServerError(resNode);
    }
}
