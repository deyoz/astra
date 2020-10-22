#include "cuws_main.h"
#include "xml_unit.h"
#include "qrys.h"
#include "astra_locale_adv.h"
#include "http_consts.h"
#include "serverlib/str_utils.h"
#include "astra_utils.h"
#include "html_pages.h"
#include <boost/regex.hpp>
#include "serverlib/xml_stuff.h" // для xml_decode_nodelist

#include "cuws_handlers.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;

namespace CUWS {

typedef void (*TCUWSHandler)(xmlNodePtr, xmlNodePtr, xmlNodePtr);
typedef pair<TCUWSHandler, string> THandler;
typedef map<string, THandler> TCUWSHandlerList;
static const TCUWSHandlerList handler_list {
    {"Get_PassengerInfo_By_BCBP",   {Get_PassengerInfo_By_BCBP, "ns:PassengerInfo"}},
    {"Issue_TagNumber",             {Issue_TagNumber,           "ns:BagSummary"}},
    {"Set_Bag_as_Active",           {Set_Bag_as_Active,         "ns:BagSummary"}},
    {"Set_Bag_as_Inactive",         {Set_Bag_as_Inactive,       "ns:BagSummary"}},
};

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

string wrap_xml_tag(const string &tag, const string &data)
{
    return "<" + tag + ">" + data + "</" + tag + ">";
}

void CUWSInternalServerError(xmlNodePtr resNode, const string &what = {}, const string &code = {})
{
    static const string faultstring = "faultstring";
    static const string detail = "detail";
    static const string tag_code = "code";
    static const string place_holder = "Internal Server Error";
    static const string full_place_holder = wrap_xml_tag(faultstring, place_holder);

    if(what.empty())
        to_content(resNode, "/cuws_error.xml");
    else if(code.empty())
        to_content(resNode, "/cuws_error.xml", place_holder, what);
    else {
        to_content(resNode, "/cuws_error.xml", full_place_holder,
                wrap_xml_tag(faultstring, what) +
                wrap_xml_tag(detail, wrap_xml_tag(tag_code, code)));
    }
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
}

void CUWSwsdl(xmlNodePtr resNode, const string &uri, const string &host, const string &client_id)
{
    if(host.empty())
        throw Exception("host is empty");
    if(client_id.empty())
        throw Exception("client_id is empty");
    if(uri.empty())
        throw Exception("uri is empty");
    to_content(resNode, "/cuws.wsdl", "ENDPOINT", "http://" + host + uri + "?" + AstraHTTP::CLIENT_ID + "=" + client_id);
}

void CUWSDispatcher(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode)
{
    xmlNodePtr curNode = reqNode->children;
    xmlNodePtr contentNode = NodeAsNodeFast("content", curNode);
    curNode = contentNode->children;
    xmlNodePtr envelopeNode = NodeAsNodeFast("Envelope", curNode);
    curNode = envelopeNode->children;
    xmlNodePtr bodyNode = NodeAsNodeFast("Body", curNode);
    xmlNodePtr actionNode = bodyNode->children;
    if(not actionNode) throw Exception("action not found");
    string action = (const char *)actionNode->name;
    auto i = handler_list.find(action);
    if(i == handler_list.end())
        throw Exception("Action not found: " + action);
    XMLDoc resDoc = XMLDoc(i->second.second.c_str());
    i->second.first(actionNode, externalSysResNode, resDoc.docPtr()->children);
    string result = GetXMLDocText(resDoc.docPtr());
    result = result.erase(0, result.find('\n') + 1); // удаляем первую строку (<?xml version=...>)
    to_envelope(resNode, result);
}

xmlNodePtr NewTextChildNoPrefix(xmlNodePtr parent, const char *name, const char *content)
{
  if (name==NULL) return NULL;
  if (content!=NULL&&*content==0) content=NULL;
  return xmlNewTextChild(parent, xmlNewNs({}, {}, {}), BAD_CAST name, BAD_CAST content);
};

void PostProcessXMLAnswer()
{
    LogTrace(TRACE5) << __func__ << " started";

    XMLRequestCtxt *xmlRC = getXmlCtxt();
    xmlNodePtr resNode = NodeAsNode("/term/answer",xmlRC->resDoc);

    std::string error_code, error_message;
    xmlNodePtr errNode = AstraLocale::selectPriorityMessage(resNode, error_code, error_message);

    if(errNode) CUWSInternalServerError(resNode, error_message, error_code);
}

} //end namespace CUWS

void CUWSInterface::CUWS(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    try {
        map<string, string> header;
        map<string, string> get_params;
        CUWS::params_from_xml(reqNode, "header", header);
        CUWS::params_from_xml(reqNode, "get_params", get_params);

        if(get_params.find("wsdl") != get_params.end()) {
            CUWS::CUWSwsdl(
                    resNode,
                    NodeAsString(AstraHTTP::URI_PATH.c_str(), reqNode),
                    header[AstraHTTP::HOST],
                    get_params[AstraHTTP::CLIENT_ID]);
        } else {
            CUWS::CUWSDispatcher(reqNode, nullptr, resNode);
        }
    } catch(Exception &E) {
        // ProgError(STDLOG, "%s: %s", __FUNCTION__, E.what());
        CUWS::CUWSInternalServerError(resNode, E.what());
    } catch(...) {
        // ProgError(STDLOG, "%s: unknown error", __FUNCTION__);
        CUWS::CUWSInternalServerError(resNode);
    }
}

